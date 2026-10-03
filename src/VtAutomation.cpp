/*******************************************************************************
** @file       VtAutomation.cpp
** @author     The Open-Agriculture Developers
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "VtAutomation.hpp"

#include "Main.hpp"
#include "NumericValueConversion.hpp"
#include "ServerMainComponent.hpp"
#include "StringEncodingConversions.hpp"

#include "isobus/hardware_integration/can_hardware_interface.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace
{
	constexpr int MAX_DEPTH = 16; ///< Object pointers may form loops; no real mask nests deeper

	juce::String hex_name(std::uint64_t name)
	{
		return juce::String::toHexString(static_cast<juce::int64>(name)).paddedLeft('0', 16).toUpperCase();
	}

	juce::String pool_state_name(isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState state)
	{
		using State = isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState;
		switch (state)
		{
			case State::None:
				return "none";
			case State::Running:
				return "parsing";
			case State::Success:
				return "parsed";
			case State::Fail:
				return "failed";
			case State::Joined:
				return "ready";
			default:
				return "unknown";
		}
	}

	juce::String formatted(double value, std::uint8_t decimals)
	{
		std::ostringstream text;
		text << std::fixed << std::setprecision(decimals) << value;
		return text.str();
	}

	int int_property(const juce::var &request, const juce::Identifier &name, int fallback)
	{
		auto value = request.getProperty(name, juce::var());
		if (value.isVoid())
		{
			return fallback;
		}
		if (!(value.isInt() || value.isInt64() || value.isDouble()))
		{
			throw VtAutomationError(name.toString().toStdString() + " must be a number");
		}
		return static_cast<int>(value);
	}
} // namespace

VtAutomation::VtAutomation(ServerMainComponent &server) :
  server(server)
{
}

juce::var VtAutomation::hello()
{
	auto result = new juce::DynamicObject();
	result->setProperty("app", ProjectInfo::projectName);
	result->setProperty("build", juce::String(AgISOVirtualTerminalApplication::getApplicationBuildInfo()));
	result->setProperty("control_protocol", 1);
	result->setProperty("vt_number", server.get_vt_number());
	result->setProperty("running", isobus::CANHardwareInterface::is_running());
	result->setProperty("working_sets", working_sets());
	return juce::var(result);
}

juce::var VtAutomation::working_sets()
{
	juce::Array<juce::var> list;
	auto active = server.get_active_working_set();
	int index = 0;

	for (const auto &workingSet : server.get_managed_working_sets())
	{
		auto entry = new juce::DynamicObject();
		auto controlFunction = workingSet->get_control_function();
		entry->setProperty("index", index);
		entry->setProperty("address", (nullptr != controlFunction) ? static_cast<int>(controlFunction->get_address()) : static_cast<int>(isobus::NULL_CAN_ADDRESS));
		entry->setProperty("name", (nullptr != controlFunction) ? hex_name(controlFunction->get_NAME().get_full_name()) : juce::String());
		entry->setProperty("pool", pool_state_name(workingSet->get_object_pool_processing_state()));
		entry->setProperty("objects", static_cast<int>(workingSet->get_object_tree().size()));
		entry->setProperty("active", workingSet == active);
		list.add(juce::var(entry));
		index++;
	}
	return juce::var(list);
}

juce::var VtAutomation::select_working_set(int index)
{
	auto workingSets = server.get_managed_working_sets();
	if ((index < 0) || (index >= static_cast<int>(workingSets.size())))
	{
		throw VtAutomationError("no working set " + std::to_string(index) + "; there are " + std::to_string(workingSets.size()));
	}
	server.change_selected_working_set(static_cast<std::uint8_t>(index));
	return state();
}

juce::var VtAutomation::state()
{
	auto result = new juce::DynamicObject();
	auto workingSet = server.get_active_working_set();
	result->setProperty("working_set", juce::var());
	result->setProperty("mask", juce::var());
	result->setProperty("soft_key_mask", juce::var());
	result->setProperty("focus", juce::var());

	if (nullptr != workingSet)
	{
		auto controlFunction = workingSet->get_control_function();
		result->setProperty("working_set", (nullptr != controlFunction) ? static_cast<int>(controlFunction->get_address()) : static_cast<int>(isobus::NULL_CAN_ADDRESS));
		auto mask = active_mask(workingSet);
		if (nullptr != mask)
		{
			auto maskEntry = new juce::DynamicObject();
			maskEntry->setProperty("id", mask->get_id());
			maskEntry->setProperty("type", type_name(mask->get_object_type()));
			result->setProperty("mask", juce::var(maskEntry));
		}
		auto softKeyMask = active_soft_key_mask(workingSet);
		if (nullptr != softKeyMask)
		{
			result->setProperty("soft_key_mask", softKeyMask->get_id());
		}
		auto focus = workingSet->get_object_focus();
		if (isobus::NULL_OBJECT_ID != focus)
		{
			result->setProperty("focus", static_cast<int>(focus));
		}
	}
	return juce::var(result);
}

juce::var VtAutomation::screen(bool allObjects, bool includeHidden)
{
	auto workingSet = active_working_set();
	auto mask = active_mask(workingSet);
	auto result = state();
	juce::Array<juce::var> objects;

	if (nullptr != mask)
	{
		collect(workingSet, mask, 0, 0, 0, allObjects, includeHidden, objects);
	}
	result.getDynamicObject()->setProperty("objects", juce::var(objects));
	result.getDynamicObject()->setProperty("soft_keys", soft_keys(workingSet, active_soft_key_mask(workingSet)));
	return result;
}

juce::var VtAutomation::object(int objectID)
{
	auto workingSet = active_working_set();
	auto found = workingSet->get_object_by_id(static_cast<std::uint16_t>(objectID));
	if (nullptr == found)
	{
		throw VtAutomationError("the active working set has no object " + std::to_string(objectID));
	}
	auto result = describe(workingSet, found);
	juce::Array<juce::var> children;
	for (std::uint16_t i = 0; i < found->get_number_children(); i++)
	{
		children.add(static_cast<int>(found->get_child_id(i)));
	}
	result.getDynamicObject()->setProperty("children", juce::var(children));
	result.getDynamicObject()->setProperty("on_screen", is_on_mask(workingSet, active_mask(workingSet), found->get_id()) || is_on_mask(workingSet, active_soft_key_mask(workingSet), found->get_id()));
	return result;
}

juce::var VtAutomation::press_down(const juce::var &request, bool softKey, Press &press)
{
	auto workingSet = active_working_set();
	auto mask = active_mask(workingSet);
	if (nullptr == mask)
	{
		throw VtAutomationError("the active working set shows no mask");
	}
	std::shared_ptr<isobus::VTObject> target;

	if (softKey)
	{
		auto softKeyMask = active_soft_key_mask(workingSet);
		if (nullptr == softKeyMask)
		{
			throw VtAutomationError("the active mask has no soft key mask");
		}
		auto position = int_property(request, "position", 0);
		if (0 != position)
		{
			if ((position < 1) || (position > softKeyMask->get_number_children()))
			{
				throw VtAutomationError("the soft key mask has positions 1.." + std::to_string(softKeyMask->get_number_children()));
			}
			target = resolve(workingSet, workingSet->get_object_by_id(softKeyMask->get_child_id(static_cast<std::uint16_t>(position - 1))));
		}
		else
		{
			auto objectID = int_property(request, "object_id", -1);
			if ((objectID < 0) || !is_on_mask(workingSet, softKeyMask, static_cast<std::uint16_t>(objectID)))
			{
				throw VtAutomationError("object " + std::to_string(objectID) + " is not a key of the active soft key mask " + std::to_string(softKeyMask->get_id()));
			}
			target = workingSet->get_object_by_id(static_cast<std::uint16_t>(objectID));
		}
		if ((nullptr == target) || (isobus::VirtualTerminalObjectType::Key != target->get_object_type()))
		{
			throw VtAutomationError("there is no key there");
		}
		press.workingSet = workingSet;
		press.objectID = target->get_id();
		press.maskObjectID = mask->get_id();
		press.keyCode = std::static_pointer_cast<isobus::Key>(target)->get_key_code();
		press.isSoftKey = true;
		server.process_macro(target, isobus::EventID::OnKeyPress, isobus::VirtualTerminalObjectType::Key, workingSet);
		server.send_soft_key_activation_message(isobus::VirtualTerminalBase::KeyActivationCode::ButtonPressedOrLatched, press.objectID, press.maskObjectID, press.keyCode, workingSet->get_control_function());
	}
	else
	{
		auto objectID = int_property(request, "object_id", -1);
		target = (objectID >= 0) ? workingSet->get_object_by_id(static_cast<std::uint16_t>(objectID)) : nullptr;
		if ((nullptr == target) || (isobus::VirtualTerminalObjectType::Button != target->get_object_type()))
		{
			throw VtAutomationError("object " + std::to_string(objectID) + " is not a button");
		}
		if (!is_on_mask(workingSet, mask, target->get_id()))
		{
			throw VtAutomationError("button " + std::to_string(objectID) + " is not on the active mask " + std::to_string(mask->get_id()));
		}
		auto button = std::static_pointer_cast<isobus::Button>(target);
		if (button->get_option(isobus::Button::Options::Disabled))
		{
			throw VtAutomationError("button " + std::to_string(objectID) + " is disabled");
		}
		press.workingSet = workingSet;
		press.objectID = target->get_id();
		press.maskObjectID = mask->get_id();
		press.keyCode = button->get_key_code();
		press.isSoftKey = false;
		server.process_macro(target, isobus::EventID::OnKeyPress, isobus::VirtualTerminalObjectType::Button, workingSet);
		server.send_button_activation_message(isobus::VirtualTerminalBase::KeyActivationCode::ButtonPressedOrLatched, press.objectID, press.maskObjectID, press.keyCode, workingSet->get_control_function());
	}
	server.set_button_held(workingSet, press.objectID, press.maskObjectID, press.keyCode, press.isSoftKey);

	auto result = new juce::DynamicObject();
	result->setProperty("object_id", press.objectID);
	result->setProperty("key_code", press.keyCode);
	result->setProperty("mask", press.maskObjectID);
	return juce::var(result);
}

void VtAutomation::press_up(const Press &press)
{
	if (nullptr == press.workingSet)
	{
		return;
	}
	auto target = press.workingSet->get_object_by_id(press.objectID);
	auto destination = press.workingSet->get_control_function();
	if (press.isSoftKey)
	{
		server.send_soft_key_activation_message(isobus::VirtualTerminalBase::KeyActivationCode::ButtonUnlatchedOrReleased, press.objectID, press.maskObjectID, press.keyCode, destination);
		server.process_macro(target, isobus::EventID::OnKeyRelease, isobus::VirtualTerminalObjectType::Key, press.workingSet);
	}
	else
	{
		server.send_button_activation_message(isobus::VirtualTerminalBase::KeyActivationCode::ButtonUnlatchedOrReleased, press.objectID, press.maskObjectID, press.keyCode, destination);
		server.process_macro(target, isobus::EventID::OnKeyRelease, isobus::VirtualTerminalObjectType::Button, press.workingSet);
	}
	server.set_button_released(press.workingSet, press.objectID, press.maskObjectID, press.keyCode, press.isSoftKey);
	server.repaint_on_next_update();
}

juce::var VtAutomation::set_input(int objectID, const juce::var &request)
{
	auto workingSet = active_working_set();
	auto mask = active_mask(workingSet);
	auto target = (objectID >= 0) ? workingSet->get_object_by_id(static_cast<std::uint16_t>(objectID)) : nullptr;
	if (nullptr == target)
	{
		throw VtAutomationError("the active working set has no object " + std::to_string(objectID));
	}
	if (!is_on_mask(workingSet, mask, target->get_id()))
	{
		throw VtAutomationError("object " + std::to_string(objectID) + " is not on the active mask");
	}
	auto client = server.get_client_control_function_for_working_set(workingSet);
	auto type = target->get_object_type();

	auto variableOf = [&workingSet](std::uint16_t reference, isobus::VirtualTerminalObjectType wanted) -> std::shared_ptr<isobus::VTObject> {
		if (isobus::NULL_OBJECT_ID == reference)
		{
			return nullptr;
		}
		auto variable = workingSet->get_object_by_id(reference);
		return ((nullptr != variable) && (wanted == variable->get_object_type())) ? variable : nullptr;
	};
	// as the input dialogs do: select the object for input, enter the value, deselect it
	auto open = [&]() {
		server.send_select_input_object_message(target->get_id(), true, true, client);
		workingSet->set_object_focus(target->get_id());
		server.process_macro(target, isobus::EventID::OnInputFieldSelection, type, workingSet);
		server.process_macro(target, isobus::EventID::OnEntryOfAValue, type, workingSet);
	};
	auto close = [&]() {
		server.send_select_input_object_message(target->get_id(), false, false, client);
		workingSet->set_object_focus(isobus::NULL_OBJECT_ID);
		server.process_macro(target, isobus::EventID::OnInputFieldDeselection, type, workingSet);
		server.repaint_on_next_update();
	};

	switch (type)
	{
		case isobus::VirtualTerminalObjectType::InputNumber:
		{
			auto number = std::static_pointer_cast<isobus::InputNumber>(target);
			if (!number->get_option2(isobus::InputNumber::Options2::Enabled))
			{
				throw VtAutomationError("input number " + std::to_string(objectID) + " is disabled");
			}
			std::int64_t raw = 0;
			auto rawValue = request.getProperty("raw", juce::var());
			auto displayed = request.getProperty("value", juce::var());
			if (!rawValue.isVoid())
			{
				raw = static_cast<juce::int64>(rawValue);
			}
			else if (!displayed.isVoid() && (0.0f != number->get_scale()))
			{
				raw = std::llround((static_cast<double>(displayed) / static_cast<double>(number->get_scale())) - static_cast<double>(number->get_offset()));
			}
			else
			{
				throw VtAutomationError("an input number takes \"value\" (as displayed) or \"raw\"");
			}
			if ((raw < static_cast<std::int64_t>(number->get_minimum_value())) || (raw > static_cast<std::int64_t>(number->get_maximum_value())))
			{
				throw VtAutomationError("raw " + std::to_string(raw) + " is outside " + std::to_string(number->get_minimum_value()) + ".." + std::to_string(number->get_maximum_value()));
			}
			auto newValue = static_cast<std::uint32_t>(raw);
			open();
			auto variable = variableOf(number->get_variable_reference(), isobus::VirtualTerminalObjectType::NumberVariable);
			if (nullptr != variable)
			{
				auto numberVariable = std::static_pointer_cast<isobus::NumberVariable>(variable);
				if (numberVariable->get_value() != newValue)
				{
					server.process_macro(target, isobus::EventID::OnEntryOfANewValue, type, workingSet);
				}
				numberVariable->set_value(newValue);
				server.send_change_numeric_value_message(variable->get_id(), newValue, client);
				server.process_macro(variable, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::NumberVariable, workingSet);
			}
			else
			{
				if (number->get_value() != newValue)
				{
					server.process_macro(target, isobus::EventID::OnEntryOfANewValue, type, workingSet);
					number->set_value(newValue);
				}
				server.send_change_numeric_value_message(target->get_id(), newValue, client);
				server.process_macro(target, isobus::EventID::OnChangeValue, type, workingSet);
			}
			close();
		}
		break;

		case isobus::VirtualTerminalObjectType::InputBoolean:
		{
			auto boolean = std::static_pointer_cast<isobus::InputBoolean>(target);
			if (!boolean->get_enabled())
			{
				throw VtAutomationError("input boolean " + std::to_string(objectID) + " is disabled");
			}
			auto wanted = int_property(request, "value", -1);
			if ((0 != wanted) && (1 != wanted))
			{
				throw VtAutomationError("an input boolean takes \"value\" 0 or 1");
			}
			auto value = static_cast<std::uint8_t>(wanted);
			auto variable = variableOf(boolean->get_variable_reference(), isobus::VirtualTerminalObjectType::NumberVariable);
			server.process_macro(target, isobus::EventID::OnEntryOfAValue, type, workingSet);
			server.process_macro(target, isobus::EventID::OnEntryOfANewValue, type, workingSet);
			if (nullptr != variable)
			{
				std::static_pointer_cast<isobus::NumberVariable>(variable)->set_value(value);
				server.send_change_numeric_value_message(variable->get_id(), value, client);
				server.process_macro(variable, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::NumberVariable, workingSet);
			}
			else
			{
				boolean->set_value(value);
				server.send_change_numeric_value_message(target->get_id(), value, client);
				server.process_macro(target, isobus::EventID::OnChangeValue, type, workingSet);
			}
			server.repaint_on_next_update();
		}
		break;

		case isobus::VirtualTerminalObjectType::InputList:
		{
			auto list = std::static_pointer_cast<isobus::InputList>(target);
			if (!list->get_option(isobus::InputList::Options::Enabled))
			{
				throw VtAutomationError("input list " + std::to_string(objectID) + " is disabled");
			}
			auto index = int_property(request, "index", -1);
			if ((index < 0) || (index >= list->get_number_children()) || (isobus::NULL_OBJECT_ID == list->get_child_id(static_cast<std::uint16_t>(index))))
			{
				throw VtAutomationError("input list " + std::to_string(objectID) + " has no item " + std::to_string(index));
			}
			open();
			auto variable = variableOf(list->get_variable_reference(), isobus::VirtualTerminalObjectType::NumberVariable);
			if (nullptr != variable)
			{
				std::static_pointer_cast<isobus::NumberVariable>(variable)->set_value(static_cast<std::uint32_t>(index));
				server.send_change_numeric_value_message(variable->get_id(), static_cast<std::uint32_t>(index), client);
				server.process_macro(variable, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::NumberVariable, workingSet);
			}
			else
			{
				if (list->get_value() != index)
				{
					server.process_macro(target, isobus::EventID::OnEntryOfANewValue, type, workingSet);
					list->set_value(static_cast<std::uint8_t>(index));
				}
				server.send_change_numeric_value_message(target->get_id(), static_cast<std::uint32_t>(index), client);
				server.process_macro(target, isobus::EventID::OnChangeValue, type, workingSet);
			}
			close();
		}
		break;

		case isobus::VirtualTerminalObjectType::InputString:
		{
			auto inputString = std::static_pointer_cast<isobus::InputString>(target);
			if (!inputString->get_enabled())
			{
				throw VtAutomationError("input string " + std::to_string(objectID) + " is disabled");
			}
			auto text = request.getProperty("text", juce::var());
			if (!text.isString())
			{
				throw VtAutomationError("an input string takes \"text\"");
			}
			const std::string newContent = text.toString().toStdString();
			// as the dialog does: pad the text to the length the pool gives it
			auto padded = [&newContent](std::size_t length) {
				std::string result = newContent;
				while (result.length() < length)
				{
					result.push_back(' ');
				}
				return result;
			};
			open();
			auto variable = variableOf(inputString->get_variable_reference(), isobus::VirtualTerminalObjectType::StringVariable);
			if (nullptr != variable)
			{
				auto stringVariable = std::static_pointer_cast<isobus::StringVariable>(variable);
				auto value = padded(stringVariable->get_value().length());
				stringVariable->set_value(value);
				server.send_change_string_value_message(variable->get_id(), value, client);
				server.process_macro(variable, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::StringVariable, workingSet);
			}
			else
			{
				auto value = padded(inputString->get_value().length());
				if (inputString->get_value() != value)
				{
					server.process_macro(target, isobus::EventID::OnEntryOfANewValue, type, workingSet);
				}
				inputString->set_value(value);
				server.send_change_string_value_message(target->get_id(), value, client);
				server.process_macro(target, isobus::EventID::OnChangeValue, type, workingSet);
			}
			close();
		}
		break;

		default:
		{
			throw VtAutomationError("object " + std::to_string(objectID) + " is a " + type_name(type).toStdString() + ", not an input object");
		}
	}
	return describe(workingSet, target);
}

juce::var VtAutomation::screenshot(const juce::String &path)
{
	if (path.isEmpty() || !juce::File::isAbsolutePath(path))
	{
		throw VtAutomationError("screenshot takes an absolute \"path\"");
	}
	juce::File file(path);
	if (!file.getParentDirectory().createDirectory())
	{
		throw VtAutomationError("cannot make the folder of " + path.toStdString());
	}
	auto image = server.capture_screen_image();
	file.deleteFile();
	juce::PNGImageFormat png;
	std::unique_ptr<juce::FileOutputStream> stream(file.createOutputStream());
	if ((nullptr == stream) || !png.writeImageToStream(image, *stream))
	{
		throw VtAutomationError("cannot write " + path.toStdString());
	}
	auto result = new juce::DynamicObject();
	result->setProperty("path", file.getFullPathName());
	result->setProperty("width", image.getWidth());
	result->setProperty("height", image.getHeight());
	return juce::var(result);
}

juce::String VtAutomation::type_name(isobus::VirtualTerminalObjectType type)
{
	using Type = isobus::VirtualTerminalObjectType;
	switch (type)
	{
		case Type::WorkingSet:
			return "WorkingSet";
		case Type::DataMask:
			return "DataMask";
		case Type::AlarmMask:
			return "AlarmMask";
		case Type::Container:
			return "Container";
		case Type::WindowMask:
			return "WindowMask";
		case Type::SoftKeyMask:
			return "SoftKeyMask";
		case Type::Key:
			return "Key";
		case Type::Button:
			return "Button";
		case Type::KeyGroup:
			return "KeyGroup";
		case Type::InputBoolean:
			return "InputBoolean";
		case Type::InputString:
			return "InputString";
		case Type::InputNumber:
			return "InputNumber";
		case Type::InputList:
			return "InputList";
		case Type::OutputString:
			return "OutputString";
		case Type::OutputNumber:
			return "OutputNumber";
		case Type::OutputList:
			return "OutputList";
		case Type::OutputLine:
			return "OutputLine";
		case Type::OutputRectangle:
			return "OutputRectangle";
		case Type::OutputEllipse:
			return "OutputEllipse";
		case Type::OutputPolygon:
			return "OutputPolygon";
		case Type::OutputMeter:
			return "OutputMeter";
		case Type::OutputLinearBarGraph:
			return "OutputLinearBarGraph";
		case Type::OutputArchedBarGraph:
			return "OutputArchedBarGraph";
		case Type::GraphicsContext:
			return "GraphicsContext";
		case Type::Animation:
			return "Animation";
		case Type::PictureGraphic:
			return "PictureGraphic";
		case Type::NumberVariable:
			return "NumberVariable";
		case Type::StringVariable:
			return "StringVariable";
		case Type::FontAttributes:
			return "FontAttributes";
		case Type::LineAttributes:
			return "LineAttributes";
		case Type::FillAttributes:
			return "FillAttributes";
		case Type::InputAttributes:
			return "InputAttributes";
		case Type::ObjectPointer:
			return "ObjectPointer";
		case Type::Macro:
			return "Macro";
		default:
			return "Type" + juce::String(static_cast<int>(type));
	}
}

// --- helpers -------------------------------------------------------------------------------------

std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> VtAutomation::active_working_set() const
{
	auto workingSet = server.get_active_working_set();
	if (nullptr == workingSet)
	{
		throw VtAutomationError("no working set is active");
	}
	return workingSet;
}

std::shared_ptr<isobus::VTObject> VtAutomation::active_mask(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet) const
{
	if (nullptr == workingSet)
	{
		return nullptr;
	}
	auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(workingSet->get_working_set_object());
	if ((nullptr == workingSetObject) || (isobus::NULL_OBJECT_ID == workingSetObject->get_active_mask()))
	{
		return nullptr;
	}
	return workingSet->get_object_by_id(workingSetObject->get_active_mask());
}

std::shared_ptr<isobus::VTObject> VtAutomation::active_soft_key_mask(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet) const
{
	auto mask = active_mask(workingSet);
	if (nullptr == mask)
	{
		return nullptr;
	}
	std::uint16_t softKeyMaskID = isobus::NULL_OBJECT_ID;
	if (isobus::VirtualTerminalObjectType::DataMask == mask->get_object_type())
	{
		softKeyMaskID = std::static_pointer_cast<isobus::DataMask>(mask)->get_soft_key_mask();
	}
	else if (isobus::VirtualTerminalObjectType::AlarmMask == mask->get_object_type())
	{
		softKeyMaskID = std::static_pointer_cast<isobus::AlarmMask>(mask)->get_soft_key_mask();
	}
	auto softKeyMask = (isobus::NULL_OBJECT_ID != softKeyMaskID) ? workingSet->get_object_by_id(softKeyMaskID) : nullptr;
	return ((nullptr != softKeyMask) && (isobus::VirtualTerminalObjectType::SoftKeyMask == softKeyMask->get_object_type())) ? softKeyMask : nullptr;
}

std::shared_ptr<isobus::VTObject> VtAutomation::resolve(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, std::shared_ptr<isobus::VTObject> object) const
{
	for (int hops = 0; (nullptr != object) && (isobus::VirtualTerminalObjectType::ObjectPointer == object->get_object_type()) && (hops < MAX_DEPTH); hops++)
	{
		auto pointed = std::static_pointer_cast<isobus::ObjectPointer>(object)->get_value();
		object = (isobus::NULL_OBJECT_ID != pointed) ? workingSet->get_object_by_id(pointed) : nullptr;
	}
	return object;
}

bool VtAutomation::is_on_mask(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const std::shared_ptr<isobus::VTObject> &mask, std::uint16_t objectID) const
{
	if (nullptr == mask)
	{
		return false;
	}
	juce::Array<juce::var> objects;
	collect(workingSet, mask, 0, 0, 0, true, false, objects);
	for (const auto &entry : objects)
	{
		if (static_cast<int>(entry.getProperty("id", -1)) == static_cast<int>(objectID))
		{
			return true;
		}
	}
	return false;
}

void VtAutomation::collect(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet,
                           const std::shared_ptr<isobus::VTObject> &object,
                           int x,
                           int y,
                           int depth,
                           bool allObjects,
                           bool includeHidden,
                           juce::Array<juce::var> &out) const
{
	using Type = isobus::VirtualTerminalObjectType;
	auto shown = resolve(workingSet, object);
	if ((nullptr == shown) || (depth > MAX_DEPTH))
	{
		return;
	}
	auto type = shown->get_object_type();
	if ((Type::Container == type) && std::static_pointer_cast<isobus::Container>(shown)->get_hidden() && !includeHidden)
	{
		return;
	}
	const bool drawing = (Type::OutputLine == type) || (Type::OutputRectangle == type) || (Type::OutputEllipse == type) || (Type::OutputPolygon == type);
	const bool mask = (Type::DataMask == type) || (Type::AlarmMask == type) || (Type::SoftKeyMask == type);
	if (!mask && (Type::Container != type) && (allObjects || !drawing))
	{
		auto entry = describe(workingSet, shown);
		entry.getDynamicObject()->setProperty("x", x);
		entry.getDynamicObject()->setProperty("y", y);
		entry.getDynamicObject()->setProperty("depth", depth);
		out.add(entry);
	}
	// a list's children are its items, of which describe() shows the selected one
	if ((Type::InputList == type) || (Type::OutputList == type))
	{
		return;
	}
	const int offset = ((Type::Button == type) && !std::static_pointer_cast<isobus::Button>(shown)->get_option(isobus::Button::Options::NoBorder)) ? 4 : 0;
	for (std::uint16_t i = 0; i < shown->get_number_children(); i++)
	{
		auto child = workingSet->get_object_by_id(shown->get_child_id(i));
		collect(workingSet, child, x + shown->get_child_x(i) + offset, y + shown->get_child_y(i) + offset, depth + 1, allObjects, includeHidden, out);
	}
}

juce::var VtAutomation::describe(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const std::shared_ptr<isobus::VTObject> &object) const
{
	using Type = isobus::VirtualTerminalObjectType;
	auto entry = new juce::DynamicObject();
	auto type = object->get_object_type();
	entry->setProperty("id", object->get_id());
	entry->setProperty("type", type_name(type));
	entry->setProperty("w", object->get_width());
	entry->setProperty("h", object->get_height());

	switch (type)
	{
		case Type::OutputString:
		case Type::InputString:
		{
			entry->setProperty("text", text_of(workingSet, object));
			if (Type::InputString == type)
			{
				entry->setProperty("enabled", std::static_pointer_cast<isobus::InputString>(object)->get_enabled());
			}
		}
		break;

		case Type::OutputNumber:
		case Type::InputNumber:
		{
			auto number = std::static_pointer_cast<isobus::NumberVTObject>(object);
			auto raw = number_raw_value(workingSet, *number);
			auto displayed = NumericValueConversion::to_displayed_value(raw, number->get_offset(), number->get_scale());
			entry->setProperty("raw", static_cast<juce::int64>(raw));
			entry->setProperty("value", displayed);
			entry->setProperty("text", formatted(displayed, number->get_number_of_decimals()));
			if (Type::InputNumber == type)
			{
				auto input = std::static_pointer_cast<isobus::InputNumber>(object);
				entry->setProperty("enabled", input->get_option2(isobus::InputNumber::Options2::Enabled));
				entry->setProperty("min", NumericValueConversion::to_displayed_value(input->get_minimum_value(), number->get_offset(), number->get_scale()));
				entry->setProperty("max", NumericValueConversion::to_displayed_value(input->get_maximum_value(), number->get_offset(), number->get_scale()));
			}
		}
		break;

		case Type::InputBoolean:
		{
			auto boolean = std::static_pointer_cast<isobus::InputBoolean>(object);
			entry->setProperty("value", static_cast<int>(variable_or(workingSet, boolean->get_variable_reference(), boolean->get_value())));
			entry->setProperty("enabled", boolean->get_enabled());
		}
		break;

		case Type::InputList:
		case Type::OutputList:
		{
			auto list = std::static_pointer_cast<isobus::ListVTObject>(object);
			auto index = variable_or(workingSet, list->get_variable_reference(), list->get_value());
			entry->setProperty("index", static_cast<int>(index));
			entry->setProperty("items", object->get_number_children());
			if (index < object->get_number_children())
			{
				auto item = resolve(workingSet, workingSet->get_object_by_id(object->get_child_id(static_cast<std::uint16_t>(index))));
				juce::Array<juce::var> pictures;
				entry->setProperty("text", (nullptr != item) ? item_text(workingSet, item, pictures) : juce::String());
			}
			if (Type::InputList == type)
			{
				entry->setProperty("enabled", std::static_pointer_cast<isobus::InputList>(object)->get_option(isobus::InputList::Options::Enabled));
			}
		}
		break;

		case Type::OutputMeter:
		{
			auto meter = std::static_pointer_cast<isobus::OutputMeter>(object);
			entry->setProperty("value", static_cast<int>(variable_or(workingSet, meter->get_variable_reference(), meter->get_value())));
		}
		break;

		case Type::OutputLinearBarGraph:
		{
			auto bar = std::static_pointer_cast<isobus::OutputLinearBarGraph>(object);
			entry->setProperty("value", static_cast<int>(variable_or(workingSet, bar->get_variable_reference(), bar->get_value())));
			entry->setProperty("target", static_cast<int>(variable_or(workingSet, bar->get_target_value_reference(), bar->get_target_value())));
		}
		break;

		case Type::OutputArchedBarGraph:
		{
			auto bar = std::static_pointer_cast<isobus::OutputArchedBarGraph>(object);
			entry->setProperty("value", static_cast<int>(variable_or(workingSet, bar->get_variable_reference(), bar->get_value())));
			entry->setProperty("target", static_cast<int>(variable_or(workingSet, bar->get_target_value_reference(), bar->get_target_value())));
		}
		break;

		case Type::Button:
		case Type::Key:
		{
			juce::Array<juce::var> pictures;
			entry->setProperty("text", texts_below(workingSet, object, pictures, 0));
			entry->setProperty("pictures", juce::var(pictures));
			if (Type::Button == type)
			{
				auto button = std::static_pointer_cast<isobus::Button>(object);
				entry->setProperty("key_code", button->get_key_code());
				entry->setProperty("enabled", !button->get_option(isobus::Button::Options::Disabled));
			}
			else
			{
				entry->setProperty("key_code", std::static_pointer_cast<isobus::Key>(object)->get_key_code());
			}
		}
		break;

		case Type::NumberVariable:
		{
			entry->setProperty("value", static_cast<juce::int64>(std::static_pointer_cast<isobus::NumberVariable>(object)->get_value()));
		}
		break;

		case Type::StringVariable:
		{
			entry->setProperty("text", juce::String(std::static_pointer_cast<isobus::StringVariable>(object)->get_value()).trimEnd());
		}
		break;

		case Type::Container:
		{
			entry->setProperty("hidden", std::static_pointer_cast<isobus::Container>(object)->get_hidden());
		}
		break;

		default:
			break;
	}
	return juce::var(entry);
}

juce::var VtAutomation::soft_keys(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const std::shared_ptr<isobus::VTObject> &softKeyMask) const
{
	juce::Array<juce::var> keys;
	if (nullptr == softKeyMask)
	{
		return juce::var(keys);
	}
	const int rows = std::max(1, static_cast<int>(server.get_physical_soft_key_rows()));
	const int columns = std::max(1, static_cast<int>(server.get_physical_soft_key_columns()));

	for (std::uint16_t i = 0; i < softKeyMask->get_number_children(); i++)
	{
		auto key = resolve(workingSet, workingSet->get_object_by_id(softKeyMask->get_child_id(i)));
		if ((nullptr == key) || (isobus::VirtualTerminalObjectType::Key != key->get_object_type()))
		{
			continue;
		}
		auto entry = describe(workingSet, key);
		entry.getDynamicObject()->setProperty("position", i + 1);
		// as the soft key area draws them: down the right-hand column first
		entry.getDynamicObject()->setProperty("row", (i % rows) + 1);
		entry.getDynamicObject()->setProperty("column", columns - (i / rows));
		keys.add(entry);
	}
	return juce::var(keys);
}

juce::String VtAutomation::text_of(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const std::shared_ptr<isobus::VTObject> &object) const
{
	auto source = std::static_pointer_cast<isobus::StringVTObject>(object);
	std::string value = source->displayed_value(workingSet->get_object_tree());
	juce::String decoded;

	if ((value.length() >= 2) && (0xFF == static_cast<std::uint8_t>(value.at(0))) && (0xFE == static_cast<std::uint8_t>(value.at(1))))
	{
		// UTF-16, as StringDrawingComponent reads it
		if (0 != (value.length() % 2))
		{
			value.pop_back();
		}
		decoded = juce::String::createStringFromData(value.c_str(), static_cast<int>(value.size()));
	}
	else
	{
		auto encoding = SourceEncoding::ISO8859_1;
		auto font = (isobus::NULL_OBJECT_ID != source->get_font_attributes()) ? workingSet->get_object_by_id(source->get_font_attributes()) : nullptr;
		if ((nullptr != font) && (isobus::VirtualTerminalObjectType::FontAttributes == font->get_object_type()))
		{
			switch (std::static_pointer_cast<isobus::FontAttributes>(font)->get_type())
			{
				case isobus::FontAttributes::FontType::ISO8859_15:
					encoding = SourceEncoding::ISO8859_15;
					break;
				case isobus::FontAttributes::FontType::ISO8859_2:
					encoding = SourceEncoding::ISO8859_2;
					break;
				case isobus::FontAttributes::FontType::ISO8859_4:
					encoding = SourceEncoding::ISO8859_4;
					break;
				case isobus::FontAttributes::FontType::ISO8859_5:
					encoding = SourceEncoding::ISO8859_5;
					break;
				case isobus::FontAttributes::FontType::ISO8859_7:
					encoding = SourceEncoding::ISO8859_7;
					break;
				default:
					break;
			}
		}
		std::string utf8;
		convert_string_to_utf_8(encoding, value, utf8, false);
		decoded = juce::String::fromUTF8(utf8.c_str(), static_cast<int>(utf8.size()));
	}
	return decoded.trimEnd();
}

juce::String VtAutomation::item_text(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const std::shared_ptr<isobus::VTObject> &item, juce::Array<juce::var> &pictures) const
{
	using Type = isobus::VirtualTerminalObjectType;
	auto type = item->get_object_type();
	if ((Type::OutputString == type) || (Type::InputString == type))
	{
		return text_of(workingSet, item);
	}
	if ((Type::OutputNumber == type) || (Type::InputNumber == type))
	{
		return describe(workingSet, item).getProperty("text", "").toString();
	}
	if (Type::PictureGraphic == type)
	{
		pictures.add(item->get_id());
		return {};
	}
	return texts_below(workingSet, item, pictures, 1);
}

juce::String VtAutomation::texts_below(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const std::shared_ptr<isobus::VTObject> &object, juce::Array<juce::var> &pictures, int depth) const
{
	juce::StringArray texts;
	if ((nullptr == object) || (depth > MAX_DEPTH))
	{
		return {};
	}
	for (std::uint16_t i = 0; i < object->get_number_children(); i++)
	{
		auto child = resolve(workingSet, workingSet->get_object_by_id(object->get_child_id(i)));
		if ((nullptr == child) ||
		    ((isobus::VirtualTerminalObjectType::Container == child->get_object_type()) && std::static_pointer_cast<isobus::Container>(child)->get_hidden()))
		{
			continue;
		}
		auto type = child->get_object_type();
		if ((isobus::VirtualTerminalObjectType::OutputString == type) || (isobus::VirtualTerminalObjectType::InputString == type) ||
		    (isobus::VirtualTerminalObjectType::OutputNumber == type) || (isobus::VirtualTerminalObjectType::InputNumber == type) ||
		    (isobus::VirtualTerminalObjectType::PictureGraphic == type))
		{
			texts.add(item_text(workingSet, child, pictures));
		}
		else
		{
			texts.add(texts_below(workingSet, child, pictures, depth + 1));
		}
	}
	texts.removeEmptyStrings();
	return texts.joinIntoString(" ");
}

std::uint32_t VtAutomation::number_raw_value(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const isobus::NumberVTObject &number) const
{
	return variable_or(workingSet, number.get_variable_reference(), number.get_value());
}

std::uint32_t VtAutomation::variable_or(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, std::uint16_t variableReference, std::uint32_t ownValue) const
{
	if (isobus::NULL_OBJECT_ID != variableReference)
	{
		auto variable = workingSet->get_object_by_id(variableReference);
		if ((nullptr != variable) && (isobus::VirtualTerminalObjectType::NumberVariable == variable->get_object_type()))
		{
			return std::static_pointer_cast<isobus::NumberVariable>(variable)->get_value();
		}
	}
	return ownValue;
}
