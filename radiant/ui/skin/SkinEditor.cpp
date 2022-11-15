#include "SkinEditor.h"

#include "i18n.h"
#include "imodelcache.h"
#include "ieclass.h"

#include <wx/dataview.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/button.h>

#include "MaterialSelectorColumn.h"
#include "SkinEditorTreeView.h"
#include "ui/modelselector/ModelTreeView.h"
#include "util/ScopedBoolLock.h"
#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"
#include "wxutil/sourceview/DeclarationSourceView.h"
#include "wxutil/sourceview/SourceView.h"

namespace ui
{

namespace
{
    constexpr const char* const DIALOG_TITLE = N_("Skin Editor");

    const std::string RKEY_ROOT = "user/ui/skinEditor/";
    const std::string RKEY_SPLIT_POS_LEFT = RKEY_ROOT + "splitPosLeft";
    const std::string RKEY_SPLIT_POS_RIGHT = RKEY_ROOT + "splitPosRight";
    const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

SkinEditor::SkinEditor() :
    DialogBase(DIALOG_TITLE),
    _selectedModels(new wxutil::TreeModel(_selectedModelColumns, true)),
    _remappings(new wxutil::TreeModel(_remappingColumns, true)),
    _controlUpdateInProgress(false),
    _skinUpdateInProgress(false)
{
    loadNamedPanel(this, "SkinEditorMainPanel");

    makeLabelBold(this, "SkinEditorSkinDefinitionsLabel");
    makeLabelBold(this, "SkinEditorEditSkinDefinitionLabel");
    makeLabelBold(this, "SkinEditorDeclarationSourceLabel");

    setupModelTreeView();
    setupSkinTreeView();
    setupSelectedModelList();
    setupRemappingPanel();
    setupPreview();

    getControl<wxButton>("SkinEditorCloseButton")->Bind(wxEVT_BUTTON, &SkinEditor::onCloseButton, this);
    getControl<wxTextCtrl>("SkinEditorSkinName")->Bind(wxEVT_TEXT, &SkinEditor::onSkinNameChanged, this);

    // Set the default size of the window
    FitToScreen(0.9f, 0.9f);

    Layout();
    Fit();

    // Connect the window position tracker
    _windowPosition.loadFromPath(RKEY_WINDOW_STATE);
    _windowPosition.connect(this);
    _windowPosition.applyPosition();

    auto leftSplitter = getControl<wxSplitterWindow>("SkinEditorLeftSplitter");
    _leftPanePosition.connect(leftSplitter);
    _leftPanePosition.loadFromPath(RKEY_SPLIT_POS_LEFT);

    auto rightSplitter = getControl<wxSplitterWindow>("SkinEditorRightSplitter");
    _rightPanePosition.connect(rightSplitter);
    _rightPanePosition.loadFromPath(RKEY_SPLIT_POS_RIGHT);

    CenterOnParent();
}

SkinEditor::~SkinEditor()
{
    _skinModifiedConn.disconnect();
}

void SkinEditor::setupModelTreeView()
{
    auto* panel = getControl<wxPanel>("SkinEditorModelTreeView");
    _modelTreeView = new ModelTreeView(panel);
    _modelTreeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SkinEditor::onModelTreeSelectionChanged, this);

    auto definitionLabel = getControl<wxStaticText>("SkinEditorModelListLabel");

    auto* treeToolbar = new wxutil::ResourceTreeViewToolbar(definitionLabel->GetParent(), _modelTreeView);
    treeToolbar->EnableFavouriteManagement(false);
    treeToolbar->SetName("ModelTreeViewToolbar");

    auto labelSizer = definitionLabel->GetContainingSizer();
    labelSizer->Replace(definitionLabel, treeToolbar);

    definitionLabel->Reparent(treeToolbar);
    treeToolbar->GetLeftSizer()->Add(definitionLabel, 0, wxALIGN_LEFT);

    panel->GetSizer()->Add(_modelTreeView, 1, wxEXPAND);
}

void SkinEditor::setupSkinTreeView()
{
    auto* panel = getControl<wxPanel>("SkinEditorSkinTreeView");
    _skinTreeView = new SkinEditorTreeView(panel, _columns, wxDV_SINGLE | wxDV_NO_HEADER);
    _skinTreeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SkinEditor::onSkinSelectionChanged, this);

    // Single visible column, containing the directory/decl name and the icon
    _skinTreeView->AppendIconTextColumn(decl::getTypeName(decl::Type::Skin), _columns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Use the TreeModel's full string search function
    _skinTreeView->AddSearchColumn(_columns.leafName);

    auto* treeToolbar = new wxutil::ResourceTreeViewToolbar(panel, _skinTreeView);
    treeToolbar->EnableFavouriteManagement(false);

    auto definitionLabel = getControl<wxStaticText>("SkinEditorSkinDefinitionsLabel");
    definitionLabel->GetContainingSizer()->Detach(definitionLabel);
    definitionLabel->Reparent(treeToolbar);
    treeToolbar->GetLeftSizer()->Add(definitionLabel, 0, wxALIGN_LEFT);

    panel->GetSizer()->Add(treeToolbar, 0, wxEXPAND | wxBOTTOM, 6);
    panel->GetSizer()->Add(_skinTreeView, 1, wxEXPAND);
}

void SkinEditor::setupSelectedModelList()
{
    auto* panel = getControl<wxPanel>("SkinEditorSelectedModelList");
    _selectedModelList = wxutil::TreeView::CreateWithModel(panel, _selectedModels.get(), wxDV_SINGLE | wxDV_NO_HEADER);
    _selectedModelList->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SkinEditor::onSkinModelSelectionChanged, this);

    // Single visible column
    _selectedModelList->AppendIconTextColumn(_("Model"), _selectedModelColumns.name.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _selectedModelList->EnableSearchPopup(false);

    auto item = panel->GetSizer()->Add(_selectedModelList, 1, wxEXPAND, 0);

    // Add a spacing to match the height of the model tree view toolbar
    auto toolbar = findNamedObject<wxWindow>(this, "ModelTreeViewToolbar");
    auto toolbarItem = toolbar->GetContainingSizer()->GetItem(toolbar);
    item->SetBorder(toolbarItem->GetSize().GetHeight() + 3);
    item->SetFlag(item->GetFlag() | wxTOP);

    auto addButton = getControl<wxWindow>("SkinEditorAddModelButton");
    addButton->Bind(wxEVT_BUTTON, &SkinEditor::onAddModelToSkin, this);

    auto removeButton = getControl<wxWindow>("SkinEditorRemoveModelButton");
    removeButton->Bind(wxEVT_BUTTON, &SkinEditor::onRemoveModelFromSkin, this);
}

void SkinEditor::setupPreview()
{
    auto panel = getControl<wxPanel>("SkinEditorPreviewPanel");
    _modelPreview = std::make_unique<wxutil::ModelPreview>(panel);
    panel->GetSizer()->Add(_modelPreview->getWidget(), 1, wxEXPAND);

    panel = getControl<wxPanel>("SkinEditorDeclarationPanel");
    _sourceView = new wxutil::D3DeclarationViewCtrl(panel);
    panel->GetSizer()->Add(_sourceView, 1, wxEXPAND);
}

void SkinEditor::setupRemappingPanel()
{
    auto panel = getControl<wxPanel>("SkinEditorRemappingPanel");

    _remappingList = wxutil::TreeView::CreateWithModel(panel, _remappings.get(), wxDV_SINGLE);

    _remappingList->AppendToggleColumn(_("Active"), _remappingColumns.active.getColumnIndex(),
        wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    auto originalColumn = new MaterialSelectorColumn(_("Original"), _remappingColumns.original.getColumnIndex());
    _remappingList->AppendColumn(originalColumn);

    auto replacementColumn = new MaterialSelectorColumn(_("Replacement"), _remappingColumns.replacement.getColumnIndex());
    _remappingList->AppendColumn(replacementColumn);

    _remappingList->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &SkinEditor::onRemappingRowChanged, this);
    _remappingList->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SkinEditor::onRemappingSelectionChanged, this);
    _remappingList->EnableSearchPopup(false);

    panel->GetSizer()->Prepend(_remappingList, 1, wxEXPAND, 0);

    auto populateButton = getControl<wxButton>("SkinEditorAddMaterialsFromModelsButton");
    populateButton->Bind(wxEVT_BUTTON, &SkinEditor::onPopulateMappingsFromModel, this);

    auto removeButton = getControl<wxButton>("SkinEditorRemoveMappingButton");
    removeButton->Bind(wxEVT_BUTTON, &SkinEditor::onRemoveSelectedMapping, this);
}

decl::ISkin::Ptr SkinEditor::getSelectedSkin()
{
    auto selectedSkin = _skinTreeView->GetSelectedDeclName();
    return selectedSkin.empty() ? decl::ISkin::Ptr() : GlobalModelSkinCache().findSkin(selectedSkin);
}

std::string SkinEditor::getSelectedModelFromTree()
{
    return _modelTreeView->GetSelectedModelPath();
}

std::string SkinEditor::getSelectedSkinModel()
{
    auto item = _selectedModelList->GetSelection();
    if (!item.IsOk()) return {};

    wxutil::TreeModel::Row row(item, *_selectedModels);
    return row[_selectedModelColumns.name].getString().ToStdString();
}

std::string SkinEditor::getSelectedRemappingSourceMaterial()
{
    auto item = _remappingList->GetSelection();
    if (!item.IsOk()) return {};

    wxutil::TreeModel::Row row(item, *_remappings);
    return row[_remappingColumns.original].getString().ToStdString();
}

void SkinEditor::updateSkinButtonSensitivity()
{
    auto selectedSkin = _skinTreeView->GetSelectedDeclName();

    getControl<wxButton>("SkinEditorCopyDefButton")->Enable(!selectedSkin.empty());
}

void SkinEditor::updateModelControlsFromSkin(const decl::ISkin::Ptr& skin)
{
    _selectedModels->Clear();

    if (!skin) return;

    for (const auto& model : skin->getModels())
    {
        auto row = _selectedModels->AddItem();
        row[_selectedModelColumns.name] = wxVariant(wxDataViewIconText(model));
        row.SendItemAdded();
    }
}

void SkinEditor::updateRemappingControlsFromSkin(const decl::ISkin::Ptr& skin)
{
    _remappings->Clear();

    if (!skin) return;

    // The wildcard item goes first
    auto wildcardRow = _remappings->AddItem();

    wildcardRow[_remappingColumns.active] = false;
    wildcardRow[_remappingColumns.original] = "*";
    wildcardRow[_remappingColumns.replacement] = "";

    wildcardRow.SendItemAdded();

    for (const auto& remapping : skin->getAllRemappings())
    {
        auto row = _remappings->AddItem();

        if (remapping.Original == "*")
        {
            wildcardRow[_remappingColumns.active] = true;
            wildcardRow[_remappingColumns.replacement] = remapping.Replacement;
            wildcardRow.SendItemChanged();
            continue;
        }

        row[_remappingColumns.active] = true;
        row[_remappingColumns.original] = remapping.Original;
        row[_remappingColumns.replacement] = remapping.Replacement;

        row.SendItemAdded();
    }

    updateRemappingButtonSensitivity();
}

void SkinEditor::updateRemappingButtonSensitivity()
{
    auto selectedSource = getSelectedRemappingSourceMaterial();
    getControl<wxButton>("SkinEditorRemoveMappingButton")->Enable(!selectedSource.empty() && selectedSource != "*");
}

void SkinEditor::updateSourceView(const decl::ISkin::Ptr& skin)
{
    if (skin)
    {
        // Surround the definition with curly braces, these are not included
        auto definition = fmt::format("{0}\n{{{1}}}", skin->getDeclName(), skin->getBlockSyntax().contents);
        _sourceView->SetValue(definition);
    }
    else
    {
        _sourceView->SetValue("");
    }

    _sourceView->Enable(skin != nullptr);
}

void SkinEditor::updateSkinControlsFromSelection()
{
    util::ScopedBoolLock lock(_controlUpdateInProgress);

    auto skin = getSelectedSkin();

    getControl<wxWindow>("SkinEditorNotebook")->Enable(skin != nullptr);
    getControl<wxWindow>("SkinEditorEditSkinDefinitionLabel")->Enable(skin != nullptr);
    getControl<wxWindow>("SkinEditorSkinNameLabel")->Enable(skin != nullptr);
    getControl<wxWindow>("SkinEditorSkinName")->Enable(skin != nullptr);

    updateSourceView(skin);
    updateModelControlsFromSkin(skin);
    updateRemappingControlsFromSkin(skin);
    updateSkinButtonSensitivity();
    updateModelButtonSensitivity();
    updateSkinTreeItem();

    auto name = skin ? skin->getDeclName() : "";
    auto nameCtrl = getControl<wxTextCtrl>("SkinEditorSkinName");

    if (nameCtrl->GetValue() != name)
    {
        nameCtrl->SetValue(name);
    }
}

void SkinEditor::updateModelButtonSensitivity()
{
    // Update the button sensitivity
    auto addButton = getControl<wxWindow>("SkinEditorAddModelButton");
    auto removeButton = getControl<wxWindow>("SkinEditorRemoveModelButton");
    auto selectedModel = getSelectedModelFromTree();
    auto selectedSkin = getSelectedSkin();

    // Add button is active if there's a skin and the selected model is not already part of the skin
    if (selectedSkin && !selectedModel.empty() && selectedSkin->getModels().count(selectedModel) == 0)
    {
        addButton->Enable();
    }
    else
    {
        addButton->Disable();
    }

    // Remove button is active if we have a selection in the skin model list
    removeButton->Enable(!getSelectedSkinModel().empty());
}

void SkinEditor::updateSkinTreeItem()
{
    if (!_skin) return;

    auto item = _skinTreeView->GetTreeModel()->FindString(_skin->getDeclName(), _columns.declName);

    if (!item.IsOk())
    {
        return;
    }

    bool isModified = _skin->isModified();

    wxutil::TreeModel::Row row(item, *_skinTreeView->GetModel());

    row[_columns.iconAndName].setAttr(!row[_columns.isFolder].getBool() ?
        wxutil::TreeViewItemStyle::Modified(isModified) : wxDataViewItemAttr()
    );

    wxDataViewIconText value = row[_columns.iconAndName];

    if (!isModified && value.GetText().EndsWith("*"))
    {
        value.SetText(value.GetText().RemoveLast(1));
        row[_columns.iconAndName] = wxVariant(value);
    }
    else if (isModified && !value.GetText().EndsWith("*"))
    {
        value.SetText(value.GetText() + "*");
        row[_columns.iconAndName] = wxVariant(value);
    }

    row.SendItemChanged();
}

void SkinEditor::onCloseButton(wxCommandEvent& ev)
{
    EndModal(wxCLOSE);
}

void SkinEditor::onSkinSelectionChanged(wxDataViewEvent& ev)
{
    if (_controlUpdateInProgress) return;

    handleSkinSelectionChanged();
}

void SkinEditor::handleSkinSelectionChanged()
{
    _skinModifiedConn.disconnect();

    _skin = getSelectedSkin();

    if (_skin)
    {
        _skinModifiedConn = _skin->signal_DeclarationChanged().connect(
            sigc::mem_fun(*this, &SkinEditor::onSkinDeclarationChanged));
    }

    updateSkinControlsFromSelection();
}

void SkinEditor::onSkinNameChanged(wxCommandEvent& ev)
{
    if (_controlUpdateInProgress) return;

    // Block declaration changed signals
    util::ScopedBoolLock lock(_skinUpdateInProgress);

    // Rename the active skin decl
    auto nameEntry = static_cast<wxTextCtrl*>(ev.GetEventObject());

    GlobalModelSkinCache().renameSkin(_skin->getDeclName(), nameEntry->GetValue().ToStdString());
    auto item = _skinTreeView->GetTreeModel()->FindString(_skin->getDeclName(), _columns.declName);

    // Make sure the item is selected again, it will be de-selected by the rename operation
    _skinTreeView->Select(item);
    _skinTreeView->EnsureVisible(item);
    handleSkinSelectionChanged(); // also updates all controls

    nameEntry->SetFocus();
}

void SkinEditor::onSkinDeclarationChanged()
{
    if (_skinUpdateInProgress) return;

    // Refresh all controls
    updateSkinControlsFromSelection();
}

void SkinEditor::onModelTreeSelectionChanged(wxDataViewEvent& ev)
{
    if (_controlUpdateInProgress) return;

    updateModelButtonSensitivity();
}

void SkinEditor::onSkinModelSelectionChanged(wxDataViewEvent& ev)
{
    if (_controlUpdateInProgress) return;

    updateModelButtonSensitivity();
}

void SkinEditor::onAddModelToSkin(wxCommandEvent& ev)
{
    if (_controlUpdateInProgress) return;

    // Block declaration changed signals
    util::ScopedBoolLock lock(_skinUpdateInProgress);

    auto skin = getSelectedSkin();
    auto model = getSelectedModelFromTree();

    if (!skin || model.empty()) return;

    skin->addModel(model);

    updateModelControlsFromSkin(skin);
    updateSkinTreeItem();

    // Select the added model
    auto modelItem = _selectedModels->FindString(model, _selectedModelColumns.name);
    _selectedModelList->Select(modelItem);
}

void SkinEditor::onRemoveModelFromSkin(wxCommandEvent& ev)
{
    if (_controlUpdateInProgress) return;

    // Block declaration changed signals
    util::ScopedBoolLock lock(_skinUpdateInProgress);

    auto skin = getSelectedSkin();
    auto model = getSelectedSkinModel();

    if (!skin || model.empty()) return;

    skin->removeModel(model);

    updateModelControlsFromSkin(skin);
    updateSkinTreeItem();
}

void SkinEditor::onRemappingRowChanged(wxDataViewEvent& ev)
{
    if (_controlUpdateInProgress || !_skin) return;

    util::ScopedBoolLock lock(_skinUpdateInProgress);

    // Load all active remapping rows into the skin
    _skin->clearRemappings();

    _remappings->ForeachNode([&](const wxutil::TreeModel::Row& row)
    {
        if (!row[_remappingColumns.active].getBool()) return;

        auto original = row[_remappingColumns.original].getString().ToStdString();
        auto replacement = row[_remappingColumns.replacement].getString().ToStdString();

        if (original == replacement) return;

        _skin->addRemapping(decl::ISkin::Remapping{ std::move(original), std::move(replacement) });
    });

    updateSkinTreeItem();
    updateSourceView(_skin);
}

void SkinEditor::onRemappingSelectionChanged(wxCommandEvent& ev)
{
    updateRemappingButtonSensitivity();
}

void SkinEditor::onRemoveSelectedMapping(wxCommandEvent& ev)
{
    if (_controlUpdateInProgress || !_skin) return;

    util::ScopedBoolLock lock(_skinUpdateInProgress);

    auto selectedSource = getSelectedRemappingSourceMaterial();

    if (selectedSource.empty()) return;

    _skin->removeRemapping(selectedSource);

    // Remove the selected item only
    auto item = _remappings->FindString(selectedSource, _remappingColumns.original);
    _remappings->RemoveItem(item);

    updateSkinTreeItem();
    updateSourceView(_skin);
}

void SkinEditor::onPopulateMappingsFromModel(wxCommandEvent& ev)
{
    if (_controlUpdateInProgress || !_skin) return;

    util::ScopedBoolLock lock(_controlUpdateInProgress);

    std::set<std::string> allMaterials;

    // Get all associated models and ask them for their materials
    for (const auto& modelPath : _skin->getModels())
    {
        // Check for modelDefs, and redirect to load the mesh instead
        auto eclass = GlobalEntityClassManager().findModel(modelPath);
        auto model = GlobalModelCache().getModel(eclass ? eclass->getMesh() : modelPath);

        if (!model) continue;

        for (const auto& material : model->getActiveMaterials())
        {
            allMaterials.insert(material);
        }
    }

    std::set<std::string> existingMappingSources;

    _remappings->ForeachNode([&](const wxutil::TreeModel::Row& row)
    {
        existingMappingSources.insert(row[_remappingColumns.original].getString().ToStdString());
    });

    // Ensure a mapping entry for each collected material
    for (const auto& material : allMaterials)
    {
        if (existingMappingSources.count(material) > 0) continue;

        auto row = _remappings->AddItem();

        row[_remappingColumns.active] = false;
        row[_remappingColumns.original] = material;
        row[_remappingColumns.replacement] = material; // use the original as replacement

        row.SendItemAdded();
    }
}

int SkinEditor::ShowModal()
{
    // Restore the position
    _windowPosition.applyPosition();

    _modelTreeView->Populate();
    _skinTreeView->Populate();

    updateSkinControlsFromSelection();

    int returnCode = DialogBase::ShowModal();

    // Tell the position tracker to save the information
    _windowPosition.saveToPath(RKEY_WINDOW_STATE);
    _leftPanePosition.saveToPath(RKEY_SPLIT_POS_LEFT);
    _rightPanePosition.saveToPath(RKEY_SPLIT_POS_RIGHT);

    return returnCode;
}

void SkinEditor::ShowDialog(const cmd::ArgumentList& args)
{
    auto* editor = new SkinEditor;

    editor->ShowModal();
    editor->Destroy();
}

}
