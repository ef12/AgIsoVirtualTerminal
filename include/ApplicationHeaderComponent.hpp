//================================================================================================
/// @file ApplicationHeaderComponent.hpp
///
/// @brief Defines the primary command and status surface for the virtual terminal.
/// @author The Open-Agriculture Developers
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef APPLICATION_HEADER_COMPONENT_HPP
#define APPLICATION_HEADER_COMPONENT_HPP

#include "JuceHeader.h"

#include <cstddef>
#include <functional>

/// @brief Displays live VT state and frequently used actions above the terminal workspace.
class ApplicationHeaderComponent : public juce::Component
{
public:
	ApplicationHeaderComponent();

	void paint(juce::Graphics &graphics) override;
	void resized() override;

	void set_start_stop_callback(std::function<void()> callback);
	void set_hardware_callback(std::function<void()> callback);
	void set_traffic_callback(std::function<void()> callback);
	void set_status(bool isRunning,
	                const juce::String &driverName,
	                std::size_t workingSetCount,
	                bool isTrafficVisible);

	static constexpr int HEIGHT = 64;

private:
	juce::TextButton startStopButton{ "START VT" };
	juce::TextButton hardwareButton{ "CAN SETUP" };
	juce::TextButton trafficButton{ "TRAFFIC" };
	std::function<void()> startStopCallback;
	std::function<void()> hardwareCallback;
	std::function<void()> trafficCallback;
	juce::String selectedDriver{ "No CAN interface" };
	std::size_t activeWorkingSets = 0;
	bool running = false;
	bool trafficVisible = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApplicationHeaderComponent)
};

#endif // APPLICATION_HEADER_COMPONENT_HPP
