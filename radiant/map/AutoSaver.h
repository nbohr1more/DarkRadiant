#pragma once

#include "iregistry.h"

#include <wx/timer.h>

/* greebo: The AutoMapSaver class lets itself being called in distinct intervals
 * and saves the map files either to snapshots or to a single yyyy.autosave.map file.
 */

namespace map
{

class AutoMapSaver : 
	public sigc::trackable,
	public wxEvtHandler
{
	// TRUE, if autosaving is enabled
	bool _enabled;

	// TRUE, if the autosaver generates snapshots
	bool _snapshotsEnabled;

	// The autosave interval stored in seconds
	unsigned long _interval;

	// The timer object that triggers the callback
	wxTimer _timer;

	std::size_t _changes;

public:
	// Constructor
	AutoMapSaver();

	// Initialises the preferences and the registrykeyobserver
	void init();

	~AutoMapSaver();

	// Starts/stops the autosave "countdown"
	void startTimer();
	void stopTimer();

	// Clears the _changes member variable that indicates how many changes have been made
	void clearChanges();

	void registryKeyChanged();

	// Adds the elements to the according preference page
	void constructPreferences();

private:
	void onRadiantShutdown();

	// This performs is called to check if the map is valid/changed/should be saved
	// and calls the save routines accordingly.
	void checkSave();

	// Saves a snapshot of the currently active map (only named maps)
	void saveSnapshot();

	// This gets called when the interval time is over
	void onIntervalReached(wxTimerEvent& ev);

}; // class AutoMapSaver

// The accessor function for the static AutoSaver instance
AutoMapSaver& AutoSaver();

} // namespace map
