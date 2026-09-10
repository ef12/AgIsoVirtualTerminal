//================================================================================================
/// @file CANTrafficMonitorWindow.hpp
///
/// @brief Defines the detached window used by the CAN traffic monitor.
/// @author The Open-Agriculture Developers
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef CAN_TRAFFIC_MONITOR_WINDOW_HPP
#define CAN_TRAFFIC_MONITOR_WINDOW_HPP

#include "CANTrafficMonitorComponent.hpp"

#include <functional>

class CANTrafficMonitorWindow : public DocumentWindow
{
public:
	CANTrafficMonitorWindow(CANTrafficMonitorComponent &content, std::function<void()> closeCallback);
	~CANTrafficMonitorWindow() override;

	void closeButtonPressed() override;
	void detach_content();

private:
	std::function<void()> closeCallback;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CANTrafficMonitorWindow)
};

#endif // CAN_TRAFFIC_MONITOR_WINDOW_HPP
