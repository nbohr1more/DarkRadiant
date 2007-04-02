#include "EffectEditor.h"

#include <gtk/gtk.h>
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "ResponseEffectLoader.h"

	namespace {
		const std::string WINDOW_TITLE = "Edit Response Effect";
		
		enum {
			EFFECT_TYPE_NAME_COL,
			EFFECT_TYPE_CAPTION_COL,
			EFFECT_TYPE_NUM_COLS
		};
	}

EffectEditor::EffectEditor(GtkWindow* parent) :
	DialogWindow(WINDOW_TITLE, parent)
{
	gtk_window_set_modal(GTK_WINDOW(_window), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(_window), 12);
	gtk_window_set_type_hint(GTK_WINDOW(_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	// Load the possible effect types
	ResponseEffectLoader loader(_effectTypes);
	GlobalEntityClassManager().forEach(loader);
	
	_effectStore = gtk_list_store_new(EFFECT_TYPE_NUM_COLS,
									  G_TYPE_STRING,
									  G_TYPE_STRING,
									  -1);

	for (ResponseEffectTypeMap::iterator i = _effectTypes.begin(); 
		  i != _effectTypes.end(); i++) 
	{
		std::string caption = i->second->getValueForKey("editor_caption");
		
		GtkTreeIter iter;
		
		gtk_list_store_append(_effectStore, &iter);
		gtk_list_store_set(_effectStore, &iter, 
						   EFFECT_TYPE_NAME_COL, i->first.c_str(),
						   EFFECT_TYPE_CAPTION_COL, caption.c_str(),
						   -1);
	}
	
	populateWindow();
}

void EffectEditor::populateWindow() {
	// Create the overall vbox
	_dialogVBox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(_window), _dialogVBox);
	
	GtkWidget* effectHBox = gtk_hbox_new(FALSE, 0);
	
	_effectTypeCombo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(_effectStore));
	g_object_unref(_effectStore); // combo box owns the GTK reference now
	
	// Add the cellrenderer for the caption
	GtkCellRenderer* captionRenderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_effectTypeCombo), captionRenderer, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_effectTypeCombo), 
								  captionRenderer, "text", EFFECT_TYPE_CAPTION_COL);
								  
	GtkWidget* effectLabel = gtkutil::LeftAlignedLabel("Effect:");
	
	gtk_box_pack_start(GTK_BOX(effectHBox), effectLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(effectHBox), 
		gtkutil::LeftAlignment(_effectTypeCombo, 12, 1.0f), 
		TRUE, TRUE, 0
	);
	
	gtk_box_pack_start(GTK_BOX(_dialogVBox), effectHBox, FALSE, FALSE, 0);
}

void EffectEditor::editEffect(StimResponse& response, const unsigned int effectIndex) {
	gtk_widget_show_all(_window);
}
