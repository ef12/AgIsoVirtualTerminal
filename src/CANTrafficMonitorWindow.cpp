//================================================================================================
/// @file CANTrafficMonitorWindow.cpp
///
/// @brief Implements the detached CAN traffic monitor window.
/// @author The Open-Agriculture Developers
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include "CANTrafficMonitorWindow.hpp"

#include <utility>

CANTrafficMonitorWindow::CANTrafficMonitorWindow(CANTrafficMonitorComponent &content, std::function<void()> closeCallback) :
  DocumentWindow("CAN Traffic Monitor",
                 Desktop::getInstance().getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId),
                 DocumentWindow::closeButton),
  closeCallback(std::move(closeCallback))
{
	setUsingNativeTitleBar(true);
	setResizable(true, true);
	setContentNonOwned(&content, false);
	centreWithSize(900, 420);
	setVisible(true);
}

CANTrafficMonitorWindow::~CANTrafficMonitorWindow()
{
	detach_content();
}

void CANTrafficMonitorWindow::closeButtonPressed()
{
	if (closeCallback)
	{
		setVisible(false);
		auto callback = closeCallback;
		MessageManager::callAsync(std::move(callback));
	}
}

void CANTrafficMonitorWindow::detach_content()
{
	clearContentComponent();
}
