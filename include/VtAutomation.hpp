//================================================================================================
/// @file VtAutomation.hpp
///
/// @brief What the control interface does with the terminal: reads the working sets, the active
/// masks and the objects on them, and presses soft keys and buttons and enters input values the
/// way an operator does with the mouse. Every function must run on the JUCE message thread, as the
/// mouse handlers do.
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef VT_AUTOMATION_HPP
#define VT_AUTOMATION_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

#include <memory>
#include <stdexcept>
#include <string>

class ServerMainComponent;

/// @brief A request the control interface cannot carry out; its text is the reply's error.
class VtAutomationError : public std::runtime_error
{
public:
	explicit VtAutomationError(const std::string &what) :
	  std::runtime_error(what)
	{
	}
};

class VtAutomation
{
public:
	/// @brief A soft key or button held down by press_down(), released by press_up().
	struct Press
	{
		std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet;
		std::uint16_t objectID = isobus::NULL_OBJECT_ID;
		std::uint16_t maskObjectID = isobus::NULL_OBJECT_ID;
		std::uint8_t keyCode = 1;
		bool isSoftKey = true;
	};

	explicit VtAutomation(ServerMainComponent &server);

	/// @brief The application, its build, whether the CAN interface runs, and the working sets.
	juce::var hello();

	/// @brief Every working set the terminal knows: index, address, NAME, pool state, active.
	juce::var working_sets();

	/// @brief Makes the working set at index (as working_sets lists them) the active one.
	juce::var select_working_set(int index);

	/// @brief The active working set, its active data or alarm mask, soft key mask and focus.
	juce::var state();

	/// @brief The active mask as the operator sees it: its objects with their absolute position,
	/// text or value, and the soft keys in the order of their positions.
	/// @param[in] allObjects Also lines, rectangles, ellipses and polygons
	/// @param[in] includeHidden Also the objects of hidden containers
	juce::var screen(bool allObjects, bool includeHidden);

	/// @brief One object of the active working set's pool, with its children's IDs.
	juce::var object(int objectID);

	/// @brief Presses a soft key of the active soft key mask (by object ID, or by its position
	/// from 1) or a button of the active mask (by object ID), as a mouse press does.
	juce::var press_down(const juce::var &request, bool softKey, Press &press);

	/// @brief Releases what press_down() pressed, as a mouse release does.
	void press_up(const Press &press);

	/// @brief Enters a value into an input object of the active mask, as the input dialogs do:
	/// "value" (the displayed value) or "raw" for an input number, "value" 0/1 for an input
	/// boolean, "index" for an input list, "text" for an input string.
	juce::var set_input(int objectID, const juce::var &request);

	/// @brief Writes the data mask and soft key area as the operator sees them to a PNG file.
	juce::var screenshot(const juce::String &path);

	/// @brief The name of an object type, e.g. "OutputNumber".
	static juce::String type_name(isobus::VirtualTerminalObjectType type);

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> active_working_set() const;
	std::shared_ptr<isobus::VTObject> active_mask(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet) const;
	std::shared_ptr<isobus::VTObject> active_soft_key_mask(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet) const;
	std::shared_ptr<isobus::VTObject> resolve(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, std::shared_ptr<isobus::VTObject> object) const;
	bool is_on_mask(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const std::shared_ptr<isobus::VTObject> &mask, std::uint16_t objectID) const;
	void collect(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet,
	             const std::shared_ptr<isobus::VTObject> &object,
	             int x,
	             int y,
	             int depth,
	             bool allObjects,
	             bool includeHidden,
	             juce::Array<juce::var> &out) const;
	juce::var describe(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const std::shared_ptr<isobus::VTObject> &object) const;
	juce::var soft_keys(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const std::shared_ptr<isobus::VTObject> &softKeyMask) const;
	juce::String text_of(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const std::shared_ptr<isobus::VTObject> &object) const;
	juce::String texts_below(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const std::shared_ptr<isobus::VTObject> &object, juce::Array<juce::var> &pictures, int depth) const;
	std::uint32_t number_raw_value(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const isobus::NumberVTObject &number) const;
	std::uint32_t variable_or(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, std::uint16_t variableReference, std::uint32_t ownValue) const;

	ServerMainComponent &server;
};

#endif // VT_AUTOMATION_HPP
