#pragma once

#include <string>
#include <map>
#include "iselection.h"
#include "icommandsystem.h"
#include "iradiant.h"
#include "wxutil/window/TransientWindow.h"
#include <wx/panel.h>

namespace wxutil { class ControlButton; }

class wxTextCtrl;

/* greebo: The dialog providing the Free Transform functionality.
 *
 * The Dialog gets notified upon selection change and updates the widget
 * sensitivity accordingly.
 *
 * If any entity is part of the selection, the scale widgets get disabled.
 */
namespace ui
{

class TransformDialog;
typedef boost::shared_ptr<TransformDialog> TransformDialogPtr;

class TransformDialog
: public wxutil::TransientWindow,
  public SelectionSystem::Observer
{
private:
	// The entry fields
	struct EntryRow
	{
		bool isRotator;
		int axis;
		int direction; // Direction (rotation only), is 1 by default
		wxTextCtrl* stepEntry;
		wxutil::ControlButton* smaller;
		wxutil::ControlButton* larger;
	};

	typedef std::map<std::string, EntryRow> EntryRowMap;
	EntryRowMap _entries;

	// The reference to the SelectionInfo (number of patches, etc.)
	const SelectionInfo& _selectionInfo;

	wxPanel* _rotatePanel;
	wxPanel* _scalePanel;

private:

	// TransientWindow callbacks
	virtual void _preShow();
	virtual void _preHide();

	/** greebo: Updates the sensitivity of the widgets according to
	 * 			the current selection.
	 */
	void update();

	// This is called to initialise the dialog window / create the widgets
	void populateWindow();

	/** greebo: Helper method that creates a single row
	 *
	 * @row: the row index of <table> where the widgets should be packed into.
	 * @isRotator: set to true if a rotator row is to be created.
	 * @axis: the axis this transformation is referring to.
	 */
	EntryRow createEntryRow(const std::string& label, wxSizer* table,
							bool isRotator, int axis);

	// Callbacks to catch the scale/rotation button clicks
	void onClickSmaller(wxCommandEvent& ev, EntryRow* row);
	void onClickLarger(wxCommandEvent& ev, EntryRow* row);

	// The callback ensuring that the step changes are written to the registry
	void onStepChanged();

public:
	// Constructor
	TransformDialog();

	/** greebo: Contains the static instance of this dialog.
	 * Constructs the instance and calls toggle() when invoked.
	 */
	static TransformDialog& Instance();

	/** greebo: SelectionSystem::Observer implementation. Gets called by
	 * the SelectionSystem upon selection change to allow updating of the
	 * widget sensitivity.
	 */
	void selectionChanged(const scene::INodePtr& node, bool isComponent);

	/** greebo: Safely disconnects this dialog from all systems
	 * 			(EventManager) also saves the window state to the registry.
	 */
	void onRadiantShutdown();

	/** greebo: The command target to connect to the EventManager.
	 */
	static void toggle(const cmd::ArgumentList& args);

private:
	static TransformDialogPtr& InstancePtr();

}; // class TransformDialog

} // namespace ui

