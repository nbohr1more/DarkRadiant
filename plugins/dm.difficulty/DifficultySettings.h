#pragma once

#include "ieclass.h"
#include "entitylib.h"
#include <map>
#include <boost/shared_ptr.hpp>
#include "wxutil/TreeModel.h"

#include "DifficultyEntity.h"
#include "Setting.h"

namespace
{
	const std::string GKEY_DIFFICULTY_LEVELS("/difficulty/numLevels");
	const std::string GKEY_DIFFICULTY_ENTITYDEF_DEFAULT("/difficulty/defaultSettingsEclass");
	const std::string GKEY_DIFFICULTY_ENTITYDEF_MAP("/difficulty/mapSettingsEclass");
	const std::string GKEY_DIFFICULTY_ENTITYDEF_MENU("/difficulty/difficultyMenuEclass");
}

namespace difficulty {

class DifficultySettings
{
public:
	struct TreeModelColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		TreeModelColumns() :
			description(add(wxutil::TreeModel::Column::String)),
			classname(add(wxutil::TreeModel::Column::String)),
			settingId(add(wxutil::TreeModel::Column::Integer)),
			isOverridden(add(wxutil::TreeModel::Column::Boolean))
		{}

		wxutil::TreeModel::Column description;
		wxutil::TreeModel::Column classname;
		wxutil::TreeModel::Column settingId;
		wxutil::TreeModel::Column isOverridden;
	};

private:
	// the difficulty level, these settings are referring to
	int _level;

	// The settings map associates classnames with spawnarg change records.
	// Multiple settings can be made for a single classname.
	typedef std::multimap<std::string, SettingPtr> SettingsMap;
	SettingsMap _settings;

	// A mapping between Setting ID and Setting (for faster lookup on GTK selection)
	typedef std::map<int, SettingPtr> SettingIdMap;
	SettingIdMap _settingIds;

	// This maps classnames to wxDataViewItems, for faster lookup
	typedef std::map<std::string, wxDataViewItem> TreeIterMap;
	TreeIterMap _iterMap;

	// The treemodel
	TreeModelColumns _columns;

	// This class holds a reference to the treemodel
	wxutil::TreeModel::Ptr _store;

public:
	// Define the difficulty level in the constructor
	DifficultySettings(int level);

	~DifficultySettings();

	const TreeModelColumns& getColumns() const;

	// Get the treestore pointer for packing into a treeview
	// Note that this class will keep a reference to the model
	wxutil::TreeModel::Ptr getTreeStore() const;

	// Returns the Setting associated with the given className
	// and carrying the given ID (returned pointer might be NULL)
	SettingPtr getSettingById(int id) const;

	// Returns the level these settings are referring to
	int getLevel() const;

	// Empties the internal structures
	void clear();

	/**
	 * greebo: Saves the given setting. The ID specifices the setting currently
	 *         highlighted in the editor (pass -1 if nothing is selected).
	 *
	 * @returns: the ID of the saved Setting, can be used to select it.
	 */
	int save(int id, const SettingPtr& setting);

	// Removes the setting with the given ID
	void deleteSetting(int id);

	// Loads the data into the given treestore
	void updateTreeModel();

	// Loads all settings (matching the internal _level) from the given entityDef.
	void parseFromEntityDef(const IEntityClassPtr& def);

	// Loads all settings (matching the internal _level) from the given entity.
	void parseFromMapEntity(Entity* entity);

	// Save the relevant settings to the given difficulty entity
	void saveToEntity(DifficultyEntity& target);

	// Returns true if the given setting is overridden by map-specific settings
	bool isOverridden(const SettingPtr& setting);

	// Clears and reloads the treemodel
	void refreshTreeModel();

private:
	// Clears the tree data
	void clearTreeModel();

	// Get the inheritance key of the given classname (for sorting in the map)
	std::string getInheritanceKey(const std::string& className);

	// Creates a new setting (and updates the internal structures)
	// This needs the classname as argument (for the internal mapping)
	SettingPtr createSetting(const std::string& className);

	/**
	 * greebo: Finds or creates a setting overruling the given <existing> one.
	 */
	SettingPtr findOrCreateOverrule(const SettingPtr& existing);

	/**
	 * greebo: Returns the TreeIter pointing to the tree element <className> in <store>.
	 * If the item is not yet existing, it gets inserted into the tree, according
	 * to its inheritance tree.
	 */
	wxDataViewItem findOrInsertClassname(const std::string& className);

	/**
	 * greebo: Inserts the given classname into the treestore, using the
	 * given <parent> iter as insertion point. If <parent> is invalid,
	 * the entry is inserted at root level.
	 */
	wxDataViewItem insertClassName(const std::string& className,
									const wxDataViewItem& parent = wxDataViewItem());

	// returns the parent eclass name for the given <className> or "" if no parent
	std::string getParentClass(const std::string& className);
};
typedef boost::shared_ptr<DifficultySettings> DifficultySettingsPtr;

} // namespace difficulty
