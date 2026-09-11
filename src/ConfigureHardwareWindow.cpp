/*******************************************************************************
** @file       ConfigureHardwareWindow.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/

#include "ConfigureHardwareWindow.hpp"

ConfigureHardwareWindow::ConfigureHardwareWindow(ServerMainComponent &parentComponent, std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &canDrivers) :
  DocumentWindow("Configure Hardware", juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), DocumentWindow::closeButton),
  parentServer(parentComponent),
  content(*this, canDrivers)
{
	setOpaque(true);
	setUsingNativeTitleBar(false);
	setTitleBarHeight(38);
	setResizable(true, true);
	setResizeLimits(360, 300, 900, 700);
	setSize(400, 320);
	content.setSize(400, 320);
	setContentNonOwned(&content, false);
}

void ConfigureHardwareWindow::closeButtonPressed()
{
	setVisible(false);
}
