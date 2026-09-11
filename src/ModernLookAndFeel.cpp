//================================================================================================
/// @file ModernLookAndFeel.cpp
///
/// @brief Implements the application-wide visual theme for the virtual terminal.
/// @author The Open-Agriculture Developers
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include "ModernLookAndFeel.hpp"

#include <algorithm>

ModernLookAndFeel::ModernLookAndFeel()
{
	setColour(juce::ResizableWindow::backgroundColourId, AppTheme::canvas());
	setColour(juce::DocumentWindow::textColourId, AppTheme::text());
	setColour(juce::Label::textColourId, AppTheme::text());
	setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
	setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);

	setColour(juce::TextButton::buttonColourId, AppTheme::surfaceRaised());
	setColour(juce::TextButton::buttonOnColourId, AppTheme::accent());
	setColour(juce::TextButton::textColourOffId, AppTheme::text());
	setColour(juce::TextButton::textColourOnId, AppTheme::canvas());
	setColour(juce::ToggleButton::textColourId, AppTheme::text());
	setColour(juce::ToggleButton::tickColourId, AppTheme::canvas());
	setColour(juce::ToggleButton::tickDisabledColourId, AppTheme::textMuted());

	setColour(juce::ComboBox::backgroundColourId, AppTheme::surfaceRaised());
	setColour(juce::ComboBox::textColourId, AppTheme::text());
	setColour(juce::ComboBox::outlineColourId, AppTheme::border());
	setColour(juce::ComboBox::arrowColourId, AppTheme::accentBright());
	setColour(juce::ComboBox::focusedOutlineColourId, AppTheme::accent());
	setColour(juce::ComboBox::buttonColourId, AppTheme::surfaceHighlight());

	setColour(juce::TextEditor::backgroundColourId, AppTheme::surfaceRaised());
	setColour(juce::TextEditor::textColourId, AppTheme::text());
	setColour(juce::TextEditor::highlightColourId, AppTheme::accent().withAlpha(0.35f));
	setColour(juce::TextEditor::highlightedTextColourId, AppTheme::text());
	setColour(juce::TextEditor::outlineColourId, AppTheme::border());
	setColour(juce::TextEditor::focusedOutlineColourId, AppTheme::accent());
	setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
	setColour(juce::CaretComponent::caretColourId, AppTheme::accentBright());

	setColour(juce::PopupMenu::backgroundColourId, AppTheme::surfaceRaised());
	setColour(juce::PopupMenu::textColourId, AppTheme::text());
	setColour(juce::PopupMenu::headerTextColourId, AppTheme::textMuted());
	setColour(juce::PopupMenu::highlightedBackgroundColourId, AppTheme::accent().withAlpha(0.22f));
	setColour(juce::PopupMenu::highlightedTextColourId, AppTheme::accentBright());
	setColour(juce::ListBox::backgroundColourId, AppTheme::surface());
	setColour(juce::ListBox::outlineColourId, AppTheme::border());
	setColour(juce::ListBox::textColourId, AppTheme::text());
	setColour(juce::TableHeaderComponent::backgroundColourId, AppTheme::surfaceRaised());
	setColour(juce::TableHeaderComponent::textColourId, AppTheme::textMuted());
	setColour(juce::TableHeaderComponent::outlineColourId, AppTheme::border());
	setColour(juce::TableHeaderComponent::highlightColourId, AppTheme::surfaceHighlight());

	setColour(juce::ScrollBar::backgroundColourId, AppTheme::canvas());
	setColour(juce::ScrollBar::trackColourId, AppTheme::canvas());
	setColour(juce::ScrollBar::thumbColourId, AppTheme::border());
	setColour(juce::Slider::backgroundColourId, AppTheme::surfaceRaised());
	setColour(juce::Slider::trackColourId, AppTheme::accent());
	setColour(juce::Slider::thumbColourId, AppTheme::accentBright());
	setColour(juce::Slider::textBoxTextColourId, AppTheme::text());
	setColour(juce::Slider::textBoxBackgroundColourId, AppTheme::surfaceRaised());
	setColour(juce::Slider::textBoxOutlineColourId, AppTheme::border());

	setColour(juce::AlertWindow::backgroundColourId, AppTheme::surface());
	setColour(juce::AlertWindow::textColourId, AppTheme::text());
	setColour(juce::AlertWindow::outlineColourId, AppTheme::border());
	setColour(juce::TooltipWindow::backgroundColourId, AppTheme::surfaceHighlight());
	setColour(juce::TooltipWindow::textColourId, AppTheme::text());
	setColour(juce::TooltipWindow::outlineColourId, AppTheme::border());
}

juce::Font ModernLookAndFeel::getTextButtonFont(juce::TextButton &, int buttonHeight)
{
	return juce::Font(static_cast<float>(juce::jmin(15, buttonHeight - 8)), juce::Font::bold);
}

juce::Font ModernLookAndFeel::getComboBoxFont(juce::ComboBox &box)
{
	return juce::Font(static_cast<float>(juce::jmin(15, box.getHeight() - 8)), juce::Font::plain);
}

juce::Font ModernLookAndFeel::getMenuBarFont(juce::MenuBarComponent &, int, const juce::String &)
{
	return juce::Font(13.5f, juce::Font::bold);
}

int ModernLookAndFeel::getDefaultMenuBarHeight()
{
	return 32;
}

void ModernLookAndFeel::drawButtonBackground(juce::Graphics &graphics,
                                             juce::Button &button,
                                             const juce::Colour &backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
	auto bounds = button.getLocalBounds().toFloat().reduced(0.75f);
	auto colour = backgroundColour;
	if (!button.isEnabled())
	{
		colour = colour.withAlpha(0.35f);
	}
	else if (shouldDrawButtonAsDown)
	{
		colour = colour.darker(0.16f);
	}
	else if (shouldDrawButtonAsHighlighted)
	{
		colour = colour.brighter(0.10f);
	}

	graphics.setColour(juce::Colours::black.withAlpha(button.isEnabled() ? 0.28f : 0.12f));
	graphics.fillRoundedRectangle(bounds.translated(0.0f, 1.5f), 6.0f);
	graphics.setColour(colour);
	graphics.fillRoundedRectangle(bounds, 6.0f);
	graphics.setColour(colour.brighter(0.24f).withAlpha(button.isEnabled() ? 0.55f : 0.18f));
	graphics.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void ModernLookAndFeel::drawButtonText(juce::Graphics &graphics,
                                       juce::TextButton &button,
                                       bool,
                                       bool shouldDrawButtonAsDown)
{
	graphics.setFont(getTextButtonFont(button, button.getHeight()));
	auto textColour = button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId);
	graphics.setColour(textColour.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.45f));
	graphics.drawFittedText(button.getButtonText(),
	                        button.getLocalBounds().reduced(7, 2).translated(0, shouldDrawButtonAsDown ? 1 : 0),
	                        juce::Justification::centred,
	                        1);
}

void ModernLookAndFeel::drawToggleButton(juce::Graphics &graphics,
                                         juce::ToggleButton &button,
                                         bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown)
{
	const float boxSize = static_cast<float>(juce::jmin(18, button.getHeight() - 4));
	const float y = (static_cast<float>(button.getHeight()) - boxSize) * 0.5f;
	auto box = juce::Rectangle<float>(1.0f, y, boxSize, boxSize);

	graphics.setColour((shouldDrawButtonAsHighlighted ? AppTheme::surfaceHighlight() : AppTheme::surfaceRaised())
	                     .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.45f));
	graphics.fillRoundedRectangle(box, 4.0f);
	graphics.setColour((button.getToggleState() ? AppTheme::accent() : AppTheme::border())
	                     .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.45f));
	graphics.drawRoundedRectangle(box, 4.0f, button.getToggleState() ? 2.0f : 1.0f);

	if (button.getToggleState())
	{
		juce::Path tick;
		tick.startNewSubPath(box.getX() + 4.0f, box.getCentreY());
		tick.lineTo(box.getX() + 7.5f, box.getBottom() - 4.5f);
		tick.lineTo(box.getRight() - 3.5f, box.getY() + 4.0f);
		graphics.setColour(AppTheme::accentBright());
		graphics.strokePath(tick, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
	}

	graphics.setColour(button.findColour(juce::ToggleButton::textColourId)
	                     .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.45f));
	graphics.setFont(13.5f);
	graphics.drawFittedText(button.getButtonText(),
	                        juce::Rectangle<int>(static_cast<int>(box.getRight()) + 8, 0, button.getWidth() - static_cast<int>(box.getRight()) - 8, button.getHeight())
	                          .translated(0, shouldDrawButtonAsDown ? 1 : 0),
	                        juce::Justification::centredLeft,
	                        1);
}

void ModernLookAndFeel::drawComboBox(juce::Graphics &graphics,
                                     int width,
                                     int height,
                                     bool isButtonDown,
                                     int buttonX,
                                     int buttonY,
                                     int buttonWidth,
                                     int buttonHeight,
                                     juce::ComboBox &box)
{
	auto bounds = juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width - 1), static_cast<float>(height - 1));
	graphics.setColour(box.findColour(juce::ComboBox::backgroundColourId));
	graphics.fillRoundedRectangle(bounds, 6.0f);

	graphics.setColour(box.findColour(box.hasKeyboardFocus(false) ? juce::ComboBox::focusedOutlineColourId : juce::ComboBox::outlineColourId)
	                     .withMultipliedAlpha(box.isEnabled() ? 1.0f : 0.45f));
	graphics.drawRoundedRectangle(bounds, 6.0f, box.hasKeyboardFocus(false) ? 1.8f : 1.0f);

	auto arrowArea = juce::Rectangle<float>(static_cast<float>(buttonX),
	                                        static_cast<float>(buttonY),
	                                        static_cast<float>(buttonWidth),
	                                        static_cast<float>(buttonHeight));
	if (isButtonDown)
	{
		graphics.setColour(AppTheme::accent().withAlpha(0.12f));
		graphics.fillRoundedRectangle(arrowArea.reduced(3.0f), 4.0f);
	}
	juce::Path arrow;
	const float arrowWidth = 9.0f;
	const float centreX = arrowArea.getCentreX();
	const float centreY = arrowArea.getCentreY();
	arrow.startNewSubPath(centreX - (arrowWidth * 0.5f), centreY - 2.0f);
	arrow.lineTo(centreX, centreY + 3.0f);
	arrow.lineTo(centreX + (arrowWidth * 0.5f), centreY - 2.0f);
	graphics.setColour(box.findColour(juce::ComboBox::arrowColourId)
	                     .withMultipliedAlpha(box.isEnabled() ? 1.0f : 0.35f));
	graphics.strokePath(arrow, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void ModernLookAndFeel::positionComboBoxText(juce::ComboBox &box, juce::Label &label)
{
	label.setBounds(11, 1, box.getWidth() - 38, box.getHeight() - 2);
	label.setFont(getComboBoxFont(box));
}

void ModernLookAndFeel::fillTextEditorBackground(juce::Graphics &graphics, int width, int height, juce::TextEditor &editor)
{
	graphics.setColour(editor.findColour(juce::TextEditor::backgroundColourId));
	graphics.fillRoundedRectangle(juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width - 1), static_cast<float>(height - 1)), 6.0f);
}

void ModernLookAndFeel::drawTextEditorOutline(juce::Graphics &graphics, int width, int height, juce::TextEditor &editor)
{
	const bool focused = editor.hasKeyboardFocus(true) && !editor.isReadOnly();
	graphics.setColour(editor.findColour(focused ? juce::TextEditor::focusedOutlineColourId : juce::TextEditor::outlineColourId)
	                     .withMultipliedAlpha(editor.isEnabled() ? 1.0f : 0.45f));
	graphics.drawRoundedRectangle(juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width - 1), static_cast<float>(height - 1)),
	                              6.0f,
	                              focused ? 1.8f : 1.0f);
}

void ModernLookAndFeel::drawMenuBarBackground(juce::Graphics &graphics,
                                              int width,
                                              int height,
                                              bool,
                                              juce::MenuBarComponent &menuBar)
{
	graphics.setColour(AppTheme::surface());
	graphics.fillRect(0, 0, width, height);
	graphics.setColour(AppTheme::border().withAlpha(0.75f));
	graphics.fillRect(0, height - 1, width, 1);
}

void ModernLookAndFeel::drawMenuBarItem(juce::Graphics &graphics,
                                        int width,
                                        int height,
                                        int,
                                        const juce::String &itemText,
                                        bool isMouseOverItem,
                                        bool isMenuOpen,
                                        bool,
                                        juce::MenuBarComponent &menuBar)
{
	if (isMouseOverItem || isMenuOpen)
	{
		graphics.setColour(isMenuOpen ? AppTheme::accent().withAlpha(0.18f) : AppTheme::surfaceHighlight());
		graphics.fillRoundedRectangle(juce::Rectangle<float>(2.0f, 3.0f, static_cast<float>(width - 4), static_cast<float>(height - 6)), 5.0f);
	}
	graphics.setColour((isMouseOverItem || isMenuOpen) ? AppTheme::text() : AppTheme::textMuted());
	graphics.setFont(getMenuBarFont(menuBar, 0, itemText));
	graphics.drawFittedText(itemText, 9, 0, width - 18, height, juce::Justification::centred, 1);
}

void ModernLookAndFeel::drawDocumentWindowTitleBar(juce::DocumentWindow &window,
                                                   juce::Graphics &graphics,
                                                   int width,
                                                   int height,
                                                   int titleSpaceX,
                                                   int titleSpaceWidth,
                                                   const juce::Image *icon,
                                                   bool drawTitleTextOnLeft)
{
	juce::ColourGradient gradient(AppTheme::surfaceRaised(), 0.0f, 0.0f, AppTheme::surface(), static_cast<float>(width), 0.0f, false);
	graphics.setGradientFill(gradient);
	graphics.fillRect(0, 0, width, height);
	graphics.setColour(AppTheme::accent().withAlpha(window.isActiveWindow() ? 0.85f : 0.25f));
	graphics.fillRect(0, height - 2, width, 2);

	int textX = titleSpaceX;
	if ((nullptr != icon) && icon->isValid())
	{
		const int iconSize = juce::jmin(height - 10, 22);
		graphics.drawImageWithin(*icon, titleSpaceX, (height - iconSize) / 2, iconSize, iconSize, juce::RectanglePlacement::centred);
		textX += iconSize + 8;
		titleSpaceWidth -= iconSize + 8;
	}

	graphics.setColour(window.findColour(juce::DocumentWindow::textColourId)
	                     .withMultipliedAlpha(window.isActiveWindow() ? 1.0f : 0.65f));
	graphics.setFont(juce::Font(14.0f, juce::Font::bold));
	graphics.drawFittedText(window.getName(),
	                        textX,
	                        0,
	                        titleSpaceWidth,
	                        height,
	                        drawTitleTextOnLeft ? juce::Justification::centredLeft : juce::Justification::centred,
	                        1);
}

void ModernLookAndFeel::drawTableHeaderBackground(juce::Graphics &graphics, juce::TableHeaderComponent &header)
{
	graphics.setColour(header.findColour(juce::TableHeaderComponent::backgroundColourId));
	graphics.fillRect(header.getLocalBounds());
	graphics.setColour(header.findColour(juce::TableHeaderComponent::outlineColourId));
	graphics.fillRect(0, header.getHeight() - 1, header.getWidth(), 1);
}

void ModernLookAndFeel::drawTableHeaderColumn(juce::Graphics &graphics,
                                              juce::TableHeaderComponent &header,
                                              const juce::String &columnName,
                                              int,
                                              int width,
                                              int height,
                                              bool isMouseOver,
                                              bool isMouseDown,
                                              int)
{
	if (isMouseOver || isMouseDown)
	{
		graphics.setColour(header.findColour(juce::TableHeaderComponent::highlightColourId));
		graphics.fillRect(0, 0, width, height);
	}
	graphics.setColour(header.findColour(juce::TableHeaderComponent::textColourId));
	graphics.setFont(juce::Font(12.0f, juce::Font::bold));
	graphics.drawFittedText(columnName.toUpperCase(), 8, 0, width - 13, height, juce::Justification::centredLeft, 1);
	graphics.setColour(header.findColour(juce::TableHeaderComponent::outlineColourId).withAlpha(0.7f));
	graphics.fillRect(width - 1, 5, 1, height - 10);
}

void ModernLookAndFeel::drawScrollbar(juce::Graphics &graphics,
                                      juce::ScrollBar &scrollBar,
                                      int x,
                                      int y,
                                      int width,
                                      int height,
                                      bool isScrollbarVertical,
                                      int thumbStartPosition,
                                      int thumbSize,
                                      bool isMouseOver,
                                      bool isMouseDown)
{
	graphics.setColour(scrollBar.findColour(juce::ScrollBar::backgroundColourId));
	graphics.fillRect(x, y, width, height);

	auto thumbColour = scrollBar.findColour(juce::ScrollBar::thumbColourId);
	if (isMouseDown)
	{
		thumbColour = AppTheme::accent();
	}
	else if (isMouseOver)
	{
		thumbColour = thumbColour.brighter(0.18f);
	}
	graphics.setColour(thumbColour);
	if (isScrollbarVertical)
	{
		graphics.fillRoundedRectangle(static_cast<float>(x + 3),
		                              static_cast<float>(thumbStartPosition),
		                              static_cast<float>(juce::jmax(3, width - 6)),
		                              static_cast<float>(thumbSize),
		                              3.0f);
	}
	else
	{
		graphics.fillRoundedRectangle(static_cast<float>(thumbStartPosition),
		                              static_cast<float>(y + 3),
		                              static_cast<float>(thumbSize),
		                              static_cast<float>(juce::jmax(3, height - 6)),
		                              3.0f);
	}
}

void ModernLookAndFeel::drawStretchableLayoutResizerBar(juce::Graphics &graphics,
                                                        int width,
                                                        int height,
                                                        bool isVerticalBar,
                                                        bool isMouseOver,
                                                        bool isMouseDragging)
{
	graphics.fillAll(AppTheme::canvas());
	const auto gripColour = isMouseDragging ? AppTheme::accentBright() : (isMouseOver ? AppTheme::accent() : AppTheme::border());
	graphics.setColour(gripColour);
	const int centreX = width / 2;
	const int centreY = height / 2;
	for (int offset = -6; offset <= 6; offset += 6)
	{
		if (isVerticalBar)
		{
			graphics.fillRoundedRectangle(static_cast<float>(centreX - 1), static_cast<float>(centreY + offset - 1), 3.0f, 3.0f, 1.5f);
		}
		else
		{
			graphics.fillRoundedRectangle(static_cast<float>(centreX + offset - 1), static_cast<float>(centreY - 1), 3.0f, 3.0f, 1.5f);
		}
	}
}
