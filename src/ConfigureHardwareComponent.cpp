/*******************************************************************************
** @file       ConfigureHardwareComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ConfigureHardwareComponent.hpp"

#include "ConfigureHardwareWindow.hpp"
#include "ServerMainComponent.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/to_string.hpp"

#ifdef JUCE_WINDOWS
#include "isobus/hardware_integration/can_api2_windows_plugin.hpp"
#include "isobus/hardware_integration/toucan_vscp_canal.hpp"
#elif JUCE_LINUX
#include "isobus/hardware_integration/socket_can_interface.hpp"
#endif

#ifdef JUCE_WINDOWS
namespace
{
	constexpr int TOUCAN_SELECTOR_ID = 3;
	constexpr int CAN_API2_SELECTOR_ID = 7;
	constexpr std::size_t TOUCAN_DRIVER_INDEX = 2;
	constexpr std::size_t CAN_API2_DRIVER_INDEX = 6;
}
#endif

ConfigureHardwareComponent::ConfigureHardwareComponent(ConfigureHardwareWindow &parent, std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &canDrivers) :
  okButton("OK"),
  parentCANDrivers(canDrivers)
{
	setSize(400, 280);
	okButton.setSize(100, 30);
	okButton.setTopLeftPosition(getWidth() / 2 - okButton.getWidth() / 2, 200);
	addAndMakeVisible(okButton);

#ifdef JUCE_WINDOWS
	hardwareInterfaceSelector.setName("Hardware Interface");
	hardwareInterfaceSelector.setTextWhenNothingSelected("Select Hardware Interface");

#ifdef ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE
	hardwareInterfaceSelector.addItemList({ "PEAK PCAN USB", "Innomaker2CAN", "TouCAN", "SysTec", "SIL vCAN", "WCAN virtual bus", "PEAK PCAN Virtual" }, 1);
#else
	hardwareInterfaceSelector.addItemList({ "PEAK PCAN USB", "Innomaker2CAN (not supported with mingw)", "TouCAN", "SysTec", "SIL vCAN", "WCAN virtual bus", "PEAK PCAN Virtual" }, 1);
#endif
	int selectedID = 1;

	for (std::uint8_t i = 0; i < parentCANDrivers.size(); i++)
	{
		if ((nullptr != parentCANDrivers.at(i)) &&
		    (parentCANDrivers.at(i) == isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0)))
		{
			selectedID = i + 1;
			break;
		}
	}
	hardwareInterfaceSelector.setSelectedId(selectedID);
	hardwareInterfaceSelector.setSize(getWidth() - 20, 30);
	hardwareInterfaceSelector.setTopLeftPosition(10, 80);
	hardwareInterfaceSelector.onChange = [this]() {
		touCANSerialEditor.setVisible(TOUCAN_SELECTOR_ID == hardwareInterfaceSelector.getSelectedId());
		canAPI2NetNameEditor.setVisible(CAN_API2_SELECTOR_ID == hardwareInterfaceSelector.getSelectedId());
		repaint();
	};
	addAndMakeVisible(hardwareInterfaceSelector);

	auto inputFilter = new TextEditor::LengthAndCharacterRestriction(10, "1234567890");
	touCANSerialEditor.setName("TouCAN Serial Number");
	touCANSerialEditor.setText(isobus::to_string(std::static_pointer_cast<isobus::TouCANPlugin>(parentCANDrivers.at(TOUCAN_DRIVER_INDEX))->get_serial_number()));
	touCANSerialEditor.setSize(getWidth() - 20, 30);
	touCANSerialEditor.setTopLeftPosition(10, 140);
	touCANSerialEditor.setInputFilter(inputFilter, true);
	addChildComponent(touCANSerialEditor);

	auto netNameFilter = new TextEditor::LengthAndCharacterRestriction(20, String());
	canAPI2NetNameEditor.setName("PCAN-Virtual Network Name");
	canAPI2NetNameEditor.setText(String(std::static_pointer_cast<isobus::CANAPI2WindowsPlugin>(parentCANDrivers.at(CAN_API2_DRIVER_INDEX))->get_net_name()));
	canAPI2NetNameEditor.setSize(getWidth() - 20, 30);
	canAPI2NetNameEditor.setTopLeftPosition(10, 140);
	canAPI2NetNameEditor.setInputFilter(netNameFilter, true);
	canAPI2NetNameEditor.setTooltip("Use the same CAN-API 2 network name in the implement simulator.");
	addChildComponent(canAPI2NetNameEditor);
	touCANSerialEditor.setVisible(TOUCAN_SELECTOR_ID == selectedID);
	canAPI2NetNameEditor.setVisible(CAN_API2_SELECTOR_ID == selectedID);
#elif JUCE_LINUX
	socketCANNameEditor.setName("SocketCAN Interface Name");
	socketCANNameEditor.setText(std::static_pointer_cast<isobus::SocketCANInterface>(parentCANDrivers.at(0))->get_device_name());
	socketCANNameEditor.setSize(getWidth() - 20, 30);
	socketCANNameEditor.setTopLeftPosition(10, 80);
	addAndMakeVisible(socketCANNameEditor);
#endif
	okButton.onClick = [this, &parent]() {
#ifdef JUCE_WINDOWS
		if (TOUCAN_SELECTOR_ID == hardwareInterfaceSelector.getSelectedId())
		{
			int serial = touCANSerialEditor.getText().trim().getIntValue();
			std::static_pointer_cast<isobus::TouCANPlugin>(parentCANDrivers.at(hardwareInterfaceSelector.getSelectedId() - 1))->reconfigure(0, static_cast<std::uint32_t>(serial));
		}
		else if (CAN_API2_SELECTOR_ID == hardwareInterfaceSelector.getSelectedId())
		{
			auto canAPI2Driver = std::static_pointer_cast<isobus::CANAPI2WindowsPlugin>(parentCANDrivers.at(CAN_API2_DRIVER_INDEX));
			if (!canAPI2Driver->configure(canAPI2NetNameEditor.getText().trim().toStdString()))
			{
				AlertWindow::showAsync(MessageBoxOptions()
				                         .withIconType(MessageBoxIconType::WarningIcon)
				                         .withTitle("Invalid PCAN-Virtual network name")
				                         .withMessage("Enter a network name containing between 1 and 20 bytes.")
				                         .withButton("OK"),
				                       nullptr);
				return;
			}
		}

		if (nullptr != isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0))
		{
			isobus::CANHardwareInterface::unassign_can_channel_frame_handler(0);
		}
		isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, parentCANDrivers.at(hardwareInterfaceSelector.getSelectedId() - 1));
		isobus::CANStackLogger::info("Updated assigned CAN driver.");
#elif JUCE_LINUX
		std::static_pointer_cast<isobus::SocketCANInterface>(parentCANDrivers.at(0))->set_name(socketCANNameEditor.getText().toStdString());
		isobus::CANStackLogger::info("Updated socket CAN interface name to: " + socketCANNameEditor.getText().toStdString());
#endif
		parent.setVisible(false);
		parent.parentServer.save_settings();
	};
}

void ConfigureHardwareComponent::paint(Graphics &graphics)
{
	auto bounds = getLocalBounds();
	graphics.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
	graphics.setColour(getLookAndFeel().findColour(Label::textColourId));
	graphics.setFont(16.0f);
#ifdef JUCE_WINDOWS
	graphics.drawFittedText("Select the CAN driver to use", 10, 10, bounds.getWidth() - 20, 54, Justification::centredTop, 3);
#elif JUCE_LINUX
	graphics.drawFittedText("Enter the name of the CAN interface to use (like \"can0\")", 10, 10, bounds.getWidth() - 20, 54, Justification::centredTop, 3);
#endif

	graphics.setFont(12.0f);

#ifdef JUCE_WINDOWS
	graphics.drawFittedText("Hardware Driver", hardwareInterfaceSelector.getBounds().getX(), hardwareInterfaceSelector.getBounds().getY() - 14, hardwareInterfaceSelector.getBounds().getWidth(), 12, Justification::centredLeft, 1);

	if (TOUCAN_SELECTOR_ID == hardwareInterfaceSelector.getSelectedId())
	{
		graphics.drawFittedText("TouCAN Serial Number", touCANSerialEditor.getBounds().getX(), touCANSerialEditor.getBounds().getY() - 14, touCANSerialEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	}
	else if (CAN_API2_SELECTOR_ID == hardwareInterfaceSelector.getSelectedId())
	{
		graphics.drawFittedText("PCAN-Virtual Network Name", canAPI2NetNameEditor.getBounds().getX(), canAPI2NetNameEditor.getBounds().getY() - 14, canAPI2NetNameEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
		graphics.drawFittedText("Use the same network name in the implement simulator. The default is PCANLight_USB.", 10, 175, getWidth() - 20, 20, Justification::centredLeft, 1);
	}
#elif JUCE_LINUX
	graphics.drawFittedText("Socket CAN Interface Name", socketCANNameEditor.getBounds().getX(), socketCANNameEditor.getBounds().getY() - 14, socketCANNameEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
#endif
}

void ConfigureHardwareComponent::resized()
{
}
