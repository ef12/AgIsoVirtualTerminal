//================================================================================================
/// @file PhysicalTerminalPanel.cpp
///
/// @brief Implements a uniformly scalable physical virtual-terminal panel.
/// @author The Open-Agriculture Developers
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include "PhysicalTerminalPanel.hpp"

#include "ModernLookAndFeel.hpp"

PhysicalTerminalPanel::PhysicalTerminalPanel()
{
	setOpaque(true);
	addAndMakeVisible(deviceFace);
}

void PhysicalTerminalPanel::paint(juce::Graphics &graphics)
{
	graphics.fillAll(AppTheme::canvas());
}

void PhysicalTerminalPanel::resized()
{
	const auto availableBounds = getLocalBounds().reduced(CONTENT_PADDING).toFloat();
	const int deviceWidth = deviceFace.getWidth();
	const int deviceHeight = deviceFace.getHeight();
	if ((deviceWidth <= 0) || (deviceHeight <= 0) || availableBounds.isEmpty())
	{
		return;
	}

	fittedZoom = juce::jmin(availableBounds.getWidth() / static_cast<float>(deviceWidth),
	                        availableBounds.getHeight() / static_cast<float>(deviceHeight));
	fittedZoom = juce::jmax(0.05f, fittedZoom);

	const float renderedWidth = static_cast<float>(deviceWidth) * fittedZoom;
	const float renderedHeight = static_cast<float>(deviceHeight) * fittedZoom;
	const float offsetX = availableBounds.getX() + ((availableBounds.getWidth() - renderedWidth) * 0.5f);
	const float offsetY = availableBounds.getY() + ((availableBounds.getHeight() - renderedHeight) * 0.5f);
	deviceFace.setTransform(juce::AffineTransform::scale(fittedZoom).translated(offsetX, offsetY));
}

juce::Component &PhysicalTerminalPanel::get_screen_component()
{
	return deviceFace.get_screen_component();
}

void PhysicalTerminalPanel::set_terminal_screen_size(int width, int height)
{
	deviceFace.set_screen_size(width, height);
	resized();
}

void PhysicalTerminalPanel::set_powered(bool isPowered)
{
	deviceFace.set_powered(isPowered);
}

float PhysicalTerminalPanel::get_fitted_zoom() const
{
	return fittedZoom;
}

PhysicalTerminalPanel::DeviceFace::DeviceFace()
{
	setOpaque(false);
	screenSurface.setInterceptsMouseClicks(false, true);
	addAndMakeVisible(screenSurface);
	set_screen_size(screenWidth, screenHeight);
}

void PhysicalTerminalPanel::DeviceFace::paint(juce::Graphics &graphics)
{
	const auto outerBounds = getLocalBounds().toFloat().reduced(1.0f);
	graphics.setColour(juce::Colours::black.withAlpha(0.58f));
	graphics.fillRoundedRectangle(outerBounds.translated(0.0f, 2.0f), 10.0f);

	juce::ColourGradient bodyGradient(juce::Colour(0xff34434e), outerBounds.getX(), outerBounds.getY(), juce::Colour(0xff17232c), outerBounds.getRight(), outerBounds.getBottom(), false);
	bodyGradient.addColour(0.52, juce::Colour(0xff22323d));
	graphics.setGradientFill(bodyGradient);
	graphics.fillRoundedRectangle(outerBounds, 10.0f);

	graphics.setColour(juce::Colour(0xff61717c).withAlpha(0.72f));
	graphics.drawRoundedRectangle(outerBounds, 10.0f, 1.0f);
	graphics.setColour(juce::Colours::black.withAlpha(0.72f));
	graphics.drawRoundedRectangle(outerBounds.reduced(2.0f), 8.0f, 1.0f);

	const auto screenBounds = screenSurface.getBounds().toFloat();
	graphics.setColour(juce::Colours::black);
	graphics.fillRoundedRectangle(screenBounds, 4.0f);
	graphics.setColour(juce::Colour(0xff63747e).withAlpha(0.60f));
	graphics.drawRoundedRectangle(screenBounds.expanded(1.0f), 5.0f, 1.0f);

	graphics.setColour(AppTheme::text().withAlpha(0.90f));
	graphics.setFont(juce::Font(11.0f, juce::Font::bold));
	graphics.drawFittedText("AGISO  ◫⌁  VT",
	                        SIDE_BEZEL + 10,
	                        5,
	                        145,
	                        TOP_BEZEL - 8,
	                        juce::Justification::centredLeft,
	                        1);

	const auto powerColour = powered ? AppTheme::success() : juce::Colour(0xff53616a);
	graphics.setColour(AppTheme::textMuted());
	graphics.setFont(9.0f);
	graphics.drawFittedText(powered ? "CAN ACTIVE" : "CAN STANDBY",
	                        getWidth() - 112,
	                        5,
	                        76,
	                        TOP_BEZEL - 8,
	                        juce::Justification::centredRight,
	                        1);
	graphics.setColour(juce::Colours::black.withAlpha(0.70f));
	graphics.fillEllipse(static_cast<float>(getWidth() - 30), 13.0f, 12.0f, 12.0f);
	graphics.setColour(powerColour.withAlpha(powered ? 0.25f : 0.10f));
	graphics.fillEllipse(static_cast<float>(getWidth() - 29), 14.0f, 10.0f, 10.0f);
	graphics.setColour(powerColour);
	graphics.fillEllipse(static_cast<float>(getWidth() - 26), 17.0f, 4.0f, 4.0f);

	const float ventY = static_cast<float>(getHeight() - 8);
	graphics.setColour(juce::Colours::black.withAlpha(0.55f));
	for (int offset = -16; offset <= 16; offset += 7)
	{
		graphics.fillEllipse(static_cast<float>(getWidth() / 2 + offset), ventY, 3.0f, 3.0f);
	}

	draw_fastener(graphics, { 6.0f, 6.0f });
	draw_fastener(graphics, { static_cast<float>(getWidth() - 6), 6.0f });
	draw_fastener(graphics, { 6.0f, static_cast<float>(getHeight() - 6) });
	draw_fastener(graphics, { static_cast<float>(getWidth() - 6), static_cast<float>(getHeight() - 6) });
}

void PhysicalTerminalPanel::DeviceFace::paintOverChildren(juce::Graphics &graphics)
{
	const auto screenBounds = screenSurface.getBounds().toFloat();
	graphics.setColour(juce::Colours::black.withAlpha(0.92f));
	graphics.drawRoundedRectangle(screenBounds, 4.0f, 2.0f);
	graphics.setColour(AppTheme::accent().withAlpha(powered ? 0.22f : 0.08f));
	graphics.drawRoundedRectangle(screenBounds.reduced(1.0f), 3.0f, 1.0f);
}

void PhysicalTerminalPanel::DeviceFace::resized()
{
	screenSurface.setBounds(SIDE_BEZEL, TOP_BEZEL, screenWidth, screenHeight);
}

juce::Component &PhysicalTerminalPanel::DeviceFace::get_screen_component()
{
	return screenSurface;
}

void PhysicalTerminalPanel::DeviceFace::set_screen_size(int width, int height)
{
	screenWidth = juce::jmax(1, width);
	screenHeight = juce::jmax(1, height);
	setSize(SIDE_BEZEL + screenWidth + SIDE_BEZEL,
	        TOP_BEZEL + screenHeight + BOTTOM_BEZEL);
}

void PhysicalTerminalPanel::DeviceFace::set_powered(bool isPowered)
{
	if (powered != isPowered)
	{
		powered = isPowered;
		repaint();
	}
}

void PhysicalTerminalPanel::DeviceFace::draw_fastener(juce::Graphics &graphics, juce::Point<float> centre) const
{
	graphics.setColour(juce::Colours::black.withAlpha(0.62f));
	graphics.fillEllipse(centre.x - 3.0f, centre.y - 3.0f, 6.0f, 6.0f);
	graphics.setColour(juce::Colour(0xff71808a).withAlpha(0.72f));
	graphics.drawEllipse(centre.x - 2.5f, centre.y - 2.5f, 5.0f, 5.0f, 0.8f);
	graphics.drawLine(centre.x - 1.5f, centre.y + 1.5f, centre.x + 1.5f, centre.y - 1.5f, 0.8f);
}
