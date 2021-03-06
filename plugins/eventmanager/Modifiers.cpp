#include "Modifiers.h"

#include "i18n.h"
#include "itextstream.h"
#include "iregistry.h"

#include <wx/event.h>
#include "wxutil/MouseButton.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

// Constructor, loads the modifier nodes from the registry
Modifiers::Modifiers() :
	_modifierState(0)
{}

void Modifiers::loadModifierDefinitions() {

	xml::NodeList modifiers = GlobalRegistry().findXPath("user/ui/input//modifiers");

	if (modifiers.size() > 0) {
		// Find all button definitions
		xml::NodeList modifierList = modifiers[0].getNamedChildren("modifier");

		if (modifierList.size() > 0) {
			rMessage() << "EventManager: Modifiers found: "
								 << modifierList.size() << "\n";
			for (std::size_t i = 0; i < modifierList.size(); ++i)
			{
				const std::string name = modifierList[i].getAttributeValue("name");

				int bitIndex;
				try {
					bitIndex = boost::lexical_cast<int>(modifierList[i].getAttributeValue("bitIndex"));
				}
				catch (boost::bad_lexical_cast e) {
					bitIndex = -1;
				}

				if (name != "" && bitIndex >= 0) {
					// Save the modifier ID into the map
					_modifierBitIndices[name] = static_cast<unsigned int>(bitIndex);
				}
				else {
					rMessage() << "EventManager: Warning: Invalid modifier definition found.\n";
				}
			}
		}
		else {
			// No Button definitions found!
			rMessage() << "EventManager: Critical: No modifiers definitions found!\n";
		}
	}
	else {
		// No Button definitions found!
		rMessage() << "EventManager: Critical: No modifiers definitions found!\n";
	}
}

unsigned int Modifiers::getModifierFlags(const std::string& modifierStr) {
	StringParts parts;
	boost::algorithm::split(parts, modifierStr, boost::algorithm::is_any_of("+"));

	// Do we have any modifiers at all?
	if (parts.size() > 0) {
		unsigned int returnValue = 0;

		// Cycle through all the modifier names and construct the bitfield
		for (unsigned int i = 0; i < parts.size(); i++) {
			if (parts[i] == "") continue;

			// Try to find the modifierBitIndex
			int bitIndex = getModifierBitIndex(parts[i]);

			// Was anything found?
			if (bitIndex >= 0) {
				unsigned int bitValue = (1 << static_cast<unsigned int>(bitIndex));
				returnValue |= bitValue;
			}
		}

		return returnValue;
	}
	else {
		return 0;
	}
}

int Modifiers::getModifierBitIndex(const std::string& modifierName) {
	ModifierBitIndexMap::iterator it = _modifierBitIndices.find(modifierName);
   	if (it != _modifierBitIndices.end()) {
   		return it->second;
   	}
   	else {
   		rMessage() << "EventManager: Warning: Modifier " << modifierName.c_str() << " not found, returning -1\n";
   		return -1;
   	}
}

// Returns a bit field with the according modifier flags set
unsigned int Modifiers::getKeyboardFlagsFromMouseButtonState(unsigned int state)
{
	unsigned int returnValue = 0;

	if (state & wxutil::MouseButton::CONTROL)
	{
    	returnValue |= (1 << getModifierBitIndex("CONTROL"));
	}

	if (state & wxutil::MouseButton::SHIFT)
	{
    	returnValue |= (1 << getModifierBitIndex("SHIFT"));
	}

	if (state & wxutil::MouseButton::ALT)
	{
    	returnValue |= (1 << getModifierBitIndex("ALT"));
	}

	return returnValue;
}

unsigned int Modifiers::getKeyboardFlags(wxKeyEvent& ev)
{
	unsigned int returnValue = 0;

	if (ev.ControlDown())
	{
    	returnValue |= (1 << getModifierBitIndex("CONTROL"));
	}

	if (ev.ShiftDown())
	{
    	returnValue |= (1 << getModifierBitIndex("SHIFT"));
	}

	if (ev.AltDown())
	{
    	returnValue |= (1 << getModifierBitIndex("ALT"));
	}

	return returnValue;
}

unsigned int Modifiers::getKeyboardFlags(wxMouseEvent& ev)
{
	unsigned int returnValue = 0;

	if (ev.ControlDown())
	{
    	returnValue |= (1 << getModifierBitIndex("CONTROL"));
	}

	if (ev.ShiftDown())
	{
    	returnValue |= (1 << getModifierBitIndex("SHIFT"));
	}

	if (ev.AltDown())
	{
    	returnValue |= (1 << getModifierBitIndex("ALT"));
	}

	return returnValue;
}

// Returns a string for the given modifier flags set (e.g. "SHIFT+CONTROL")
std::string Modifiers::getModifierStr(const unsigned int modifierFlags, bool forMenu) {
	std::string returnValue = "";

	const std::string controlStr = (forMenu) ? _("Ctrl") : "CONTROL";
	const std::string shiftStr = (forMenu) ? _("Shift") : "SHIFT";
	const std::string altStr = (forMenu) ? _("Alt") : "ALT";
	const std::string connector = (forMenu) ? "-" : "+";

	if ((modifierFlags & (1 << getModifierBitIndex("CONTROL"))) != 0) {
		returnValue += (returnValue != "") ? connector : "";
		returnValue += controlStr;
	}

	if ((modifierFlags & (1 << getModifierBitIndex("SHIFT"))) != 0) {
		returnValue += (returnValue != "") ? connector : "";
		returnValue += shiftStr;
	}

	if ((modifierFlags & (1 << getModifierBitIndex("ALT"))) != 0) {
		returnValue += (returnValue != "") ? connector : "";
		returnValue += altStr;
	}

	return returnValue;
}

unsigned int Modifiers::getState() const
{
	return _modifierState;
}

void Modifiers::clearState()
{
	_modifierState = 0;
}

void Modifiers::updateState(wxKeyEvent& ev, bool keyPress)
{
	_modifierState = getKeyboardFlags(ev);
#if 0
	unsigned int mask = 0;

	unsigned int ctrlMask = 1 << getModifierBitIndex("CONTROL");
	unsigned int shiftMask = 1 << getModifierBitIndex("SHIFT");
	unsigned int altMask = 1 << getModifierBitIndex("ALT");

	mask |= (ev.ControlDown()) ? ctrlMask : 0;
	mask |= (ev.ShiftDown()) ? shiftMask : 0;
	mask |= (ev.AltDown()) ? altMask : 0;

	if (keyPress)
	{
		_modifierState |= mask;
	}
	else
	{
		_modifierState &= ~mask;
	}
#endif
}
