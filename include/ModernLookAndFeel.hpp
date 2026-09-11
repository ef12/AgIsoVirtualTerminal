//================================================================================================
/// @file ModernLookAndFeel.hpp
///
/// @brief Defines the application-wide visual theme for the virtual terminal.
/// @author The Open-Agriculture Developers
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef MODERN_LOOK_AND_FEEL_HPP
#define MODERN_LOOK_AND_FEEL_HPP

#include "JuceHeader.h"

namespace AppTheme
{
	inline juce::Colour canvas()
	{
		return juce::Colour(0xff091018);
	}

	inline juce::Colour surface()
	{
		return juce::Colour(0xff111a24);
	}

	inline juce::Colour surfaceRaised()
	{
		return juce::Colour(0xff182430);
	}

	inline juce::Colour surfaceHighlight()
	{
		return juce::Colour(0xff203140);
	}

	inline juce::Colour border()
	{
		return juce::Colour(0xff2a3c4b);
	}

	inline juce::Colour text()
	{
		return juce::Colour(0xffeaf2f8);
	}

	inline juce::Colour textMuted()
	{
		return juce::Colour(0xff8fa5b7);
	}

	inline juce::Colour accent()
	{
		return juce::Colour(0xff22c7b8);
	}

	inline juce::Colour accentBright()
	{
		return juce::Colour(0xff69e3d7);
	}

	inline juce::Colour success()
	{
		return juce::Colour(0xff48d17d);
	}

	inline juce::Colour warning()
	{
		return juce::Colour(0xffffbf5b);
	}

	inline juce::Colour danger()
	{
		return juce::Colour(0xffff6572);
	}

	inline juce::Colour receive()
	{
		return juce::Colour(0xff70e6a2);
	}

	inline juce::Colour transmit()
	{
		return juce::Colour(0xff78c7ff);
	}
}

/// @brief A compact dark theme tailored to the virtual terminal's desktop tooling.
class ModernLookAndFeel : public juce::LookAndFeel_V4
{
public:
	ModernLookAndFeel();

	juce::Font getTextButtonFont(juce::TextButton &button, int buttonHeight) override;
	juce::Font getComboBoxFont(juce::ComboBox &box) override;
	juce::Font getMenuBarFont(juce::MenuBarComponent &menuBar, int itemIndex, const juce::String &itemText) override;
	int getDefaultMenuBarHeight() override;

	void drawButtonBackground(juce::Graphics &graphics,
	                          juce::Button &button,
	                          const juce::Colour &backgroundColour,
	                          bool shouldDrawButtonAsHighlighted,
	                          bool shouldDrawButtonAsDown) override;
	void drawButtonText(juce::Graphics &graphics,
	                    juce::TextButton &button,
	                    bool shouldDrawButtonAsHighlighted,
	                    bool shouldDrawButtonAsDown) override;
	void drawToggleButton(juce::Graphics &graphics,
	                      juce::ToggleButton &button,
	                      bool shouldDrawButtonAsHighlighted,
	                      bool shouldDrawButtonAsDown) override;
	void drawComboBox(juce::Graphics &graphics,
	                  int width,
	                  int height,
	                  bool isButtonDown,
	                  int buttonX,
	                  int buttonY,
	                  int buttonWidth,
	                  int buttonHeight,
	                  juce::ComboBox &box) override;
	void positionComboBoxText(juce::ComboBox &box, juce::Label &label) override;
	void fillTextEditorBackground(juce::Graphics &graphics, int width, int height, juce::TextEditor &editor) override;
	void drawTextEditorOutline(juce::Graphics &graphics, int width, int height, juce::TextEditor &editor) override;
	void drawMenuBarBackground(juce::Graphics &graphics,
	                           int width,
	                           int height,
	                           bool isMouseOverBar,
	                           juce::MenuBarComponent &menuBar) override;
	void drawMenuBarItem(juce::Graphics &graphics,
	                     int width,
	                     int height,
	                     int itemIndex,
	                     const juce::String &itemText,
	                     bool isMouseOverItem,
	                     bool isMenuOpen,
	                     bool isMouseOverBar,
	                     juce::MenuBarComponent &menuBar) override;
	void drawDocumentWindowTitleBar(juce::DocumentWindow &window,
	                                juce::Graphics &graphics,
	                                int width,
	                                int height,
	                                int titleSpaceX,
	                                int titleSpaceWidth,
	                                const juce::Image *icon,
	                                bool drawTitleTextOnLeft) override;
	void drawTableHeaderBackground(juce::Graphics &graphics, juce::TableHeaderComponent &header) override;
	void drawTableHeaderColumn(juce::Graphics &graphics,
	                           juce::TableHeaderComponent &header,
	                           const juce::String &columnName,
	                           int columnId,
	                           int width,
	                           int height,
	                           bool isMouseOver,
	                           bool isMouseDown,
	                           int columnFlags) override;
	void drawScrollbar(juce::Graphics &graphics,
	                   juce::ScrollBar &scrollBar,
	                   int x,
	                   int y,
	                   int width,
	                   int height,
	                   bool isScrollbarVertical,
	                   int thumbStartPosition,
	                   int thumbSize,
	                   bool isMouseOver,
	                   bool isMouseDown) override;
	void drawStretchableLayoutResizerBar(juce::Graphics &graphics,
	                                     int width,
	                                     int height,
	                                     bool isVerticalBar,
	                                     bool isMouseOver,
	                                     bool isMouseDragging) override;

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernLookAndFeel)
};

#endif // MODERN_LOOK_AND_FEEL_HPP
