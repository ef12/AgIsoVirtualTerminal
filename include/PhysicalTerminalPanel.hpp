//================================================================================================
/// @file PhysicalTerminalPanel.hpp
///
/// @brief Defines a uniformly scalable physical virtual-terminal panel.
/// @author The Open-Agriculture Developers
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef PHYSICAL_TERMINAL_PANEL_HPP
#define PHYSICAL_TERMINAL_PANEL_HPP

#include "JuceHeader.h"

/// @brief Displays all VT screen regions on one fixed device face and scales them as a unit.
class PhysicalTerminalPanel : public juce::Component
{
public:
	PhysicalTerminalPanel();

	void paint(juce::Graphics &graphics) override;
	void resized() override;

	/// @returns The single screen surface that owns the working set, data mask and soft keys.
	juce::Component &get_screen_component();

	/// @brief Changes the unscaled screen dimensions and refits the complete device.
	void set_terminal_screen_size(int width, int height);

	/// @brief Changes the device power indicator without changing any CAN state.
	void set_powered(bool isPowered);

	float get_fitted_zoom() const;

	static constexpr int SIDE_BEZEL = 10;
	static constexpr int TOP_BEZEL = 38;
	static constexpr int BOTTOM_BEZEL = 16;
	static constexpr int CONTENT_PADDING = 8;

private:
	class DeviceFace : public juce::Component
	{
	public:
		DeviceFace();

		void paint(juce::Graphics &graphics) override;
		void paintOverChildren(juce::Graphics &graphics) override;
		void resized() override;

		juce::Component &get_screen_component();
		void set_screen_size(int width, int height);
		void set_powered(bool isPowered);

	private:
		void draw_fastener(juce::Graphics &graphics, juce::Point<float> centre) const;

		juce::Component screenSurface;
		bool powered = false;
		int screenWidth = 640;
		int screenHeight = 480;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceFace)
	};

	DeviceFace deviceFace;
	float fittedZoom = 1.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhysicalTerminalPanel)
};

#endif // PHYSICAL_TERMINAL_PANEL_HPP
