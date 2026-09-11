//================================================================================================
/// @file ApplicationHeaderComponent.cpp
///
/// @brief Implements the primary command and status surface for the virtual terminal.
/// @author The Open-Agriculture Developers
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include "ApplicationHeaderComponent.hpp"

#include "ModernLookAndFeel.hpp"

#include <utility>

ApplicationHeaderComponent::ApplicationHeaderComponent()
{
	startStopButton.setTooltip("Start or stop the virtual terminal CAN interface");
	hardwareButton.setTooltip("Select and configure the CAN interface");
	trafficButton.setTooltip("Show or hide the live CAN traffic monitor");
	startStopButton.onClick = [this]() {
		if (startStopCallback)
		{
			startStopCallback();
		}
	};
	hardwareButton.onClick = [this]() {
		if (hardwareCallback)
		{
			hardwareCallback();
		}
	};
	trafficButton.onClick = [this]() {
		if (trafficCallback)
		{
			trafficCallback();
		}
	};

	addAndMakeVisible(startStopButton);
#if JUCE_WINDOWS || JUCE_LINUX
	addAndMakeVisible(hardwareButton);
#else
	hardwareButton.setVisible(false);
#endif
	addAndMakeVisible(trafficButton);
	set_status(false, selectedDriver, 0, false);
}

void ApplicationHeaderComponent::paint(juce::Graphics &graphics)
{
	const auto bounds = getLocalBounds();
	juce::ColourGradient gradient(AppTheme::surfaceRaised(), 0.0f, 0.0f, AppTheme::surface(), static_cast<float>(getWidth()), static_cast<float>(getHeight()), false);
	graphics.setGradientFill(gradient);
	graphics.fillRect(bounds);
	graphics.setColour(AppTheme::border().withAlpha(0.8f));
	graphics.fillRect(0, getHeight() - 1, getWidth(), 1);

	auto logoBounds = juce::Rectangle<float>(14.0f, 12.0f, 40.0f, 40.0f);
	graphics.setColour(AppTheme::accent().withAlpha(0.16f));
	graphics.fillRoundedRectangle(logoBounds, 10.0f);
	graphics.setColour(AppTheme::accent());
	graphics.drawRoundedRectangle(logoBounds, 10.0f, 1.4f);

	const auto nodeRadius = 3.0f;
	const juce::Point<float> leftNode(logoBounds.getX() + 10.0f, logoBounds.getCentreY());
	const juce::Point<float> centreNode(logoBounds.getCentreX(), logoBounds.getY() + 10.0f);
	const juce::Point<float> rightNode(logoBounds.getRight() - 10.0f, logoBounds.getCentreY());
	const juce::Point<float> bottomNode(logoBounds.getCentreX(), logoBounds.getBottom() - 9.0f);
	graphics.setColour(AppTheme::accentBright());
	graphics.drawLine(leftNode.x, leftNode.y, centreNode.x, centreNode.y, 1.6f);
	graphics.drawLine(centreNode.x, centreNode.y, rightNode.x, rightNode.y, 1.6f);
	graphics.drawLine(centreNode.x, centreNode.y, bottomNode.x, bottomNode.y, 1.6f);
	for (const auto point : { leftNode, centreNode, rightNode, bottomNode })
	{
		graphics.fillEllipse(point.x - nodeRadius, point.y - nodeRadius, nodeRadius * 2.0f, nodeRadius * 2.0f);
	}

	const bool compact = getWidth() < 620;
	const bool narrow = getWidth() < 470;
	const int titleX = 66;
	graphics.setColour(AppTheme::text());
	graphics.setFont(juce::Font(compact ? 15.0f : 16.5f, juce::Font::bold));
	const juce::String title = narrow ? "VT" : (compact ? "VIRTUAL TERMINAL" : "ISO 11783 VIRTUAL TERMINAL");
	const int titleWidth = narrow ? 48 : (compact ? 140 : 225);
	graphics.drawFittedText(title,
	                        titleX,
	                        10,
	                        titleWidth,
	                        22,
	                        juce::Justification::centredLeft,
	                        1);

	if (!compact)
	{
		graphics.setColour(AppTheme::textMuted());
		graphics.setFont(11.5f);
		graphics.drawFittedText("Open-Agriculture test console", titleX, 33, 225, 18, juce::Justification::centredLeft, 1);
	}

	const int buttonAreaLeft = hardwareButton.isVisible() ? juce::jmin(startStopButton.getX(), hardwareButton.getX()) : juce::jmin(startStopButton.getX(), trafficButton.getX());
	const int statusLeft = titleX + titleWidth + 9;
	const int availableStatusWidth = buttonAreaLeft - statusLeft - 12;
	if (availableStatusWidth > 105)
	{
		const auto stateColour = running ? AppTheme::success() : AppTheme::warning();
		const juce::String stateText = running ? "ONLINE" : "STANDBY";
		auto statePill = juce::Rectangle<float>(static_cast<float>(statusLeft), 14.0f, running ? 74.0f : 82.0f, 22.0f);
		graphics.setColour(stateColour.withAlpha(0.13f));
		graphics.fillRoundedRectangle(statePill, 11.0f);
		graphics.setColour(stateColour.withAlpha(0.65f));
		graphics.drawRoundedRectangle(statePill, 11.0f, 1.0f);
		graphics.setColour(stateColour);
		graphics.fillEllipse(statePill.getX() + 9.0f, statePill.getCentreY() - 3.0f, 6.0f, 6.0f);
		graphics.setFont(juce::Font(11.0f, juce::Font::bold));
		graphics.drawFittedText(stateText,
		                        static_cast<int>(statePill.getX()) + 20,
		                        static_cast<int>(statePill.getY()),
		                        static_cast<int>(statePill.getWidth()) - 25,
		                        static_cast<int>(statePill.getHeight()),
		                        juce::Justification::centredLeft,
		                        1);

		if (availableStatusWidth > 215)
		{
			const int detailsX = static_cast<int>(statePill.getRight()) + 12;
			graphics.setColour(AppTheme::textMuted());
			graphics.setFont(11.5f);
			graphics.drawFittedText(selectedDriver,
			                        detailsX,
			                        9,
			                        buttonAreaLeft - detailsX - 10,
			                        21,
			                        juce::Justification::centredLeft,
			                        1);
			graphics.drawFittedText(juce::String(activeWorkingSets) + (1 == activeWorkingSets ? " working set" : " working sets"),
			                        detailsX,
			                        31,
			                        buttonAreaLeft - detailsX - 10,
			                        18,
			                        juce::Justification::centredLeft,
			                        1);
		}
	}
}

void ApplicationHeaderComponent::resized()
{
	const bool compact = getWidth() < 620;
	const int buttonHeight = 36;
	const int buttonY = (getHeight() - buttonHeight) / 2;
	const int gap = 7;
	int right = getWidth() - 12;

	const int startWidth = compact ? 76 : 96;
	startStopButton.setBounds(right - startWidth, buttonY, startWidth, buttonHeight);
	right -= startWidth + gap;
	const int secondaryWidth = compact ? 66 : 88;
	trafficButton.setBounds(right - secondaryWidth, buttonY, secondaryWidth, buttonHeight);
	right -= secondaryWidth + gap;
	if (hardwareButton.isVisible())
	{
		hardwareButton.setBounds(right - secondaryWidth, buttonY, secondaryWidth, buttonHeight);
	}

	hardwareButton.setButtonText(compact ? "CAN" : "CAN SETUP");
	trafficButton.setButtonText(compact ? "TRACE" : "TRAFFIC");
}

void ApplicationHeaderComponent::set_start_stop_callback(std::function<void()> callback)
{
	startStopCallback = std::move(callback);
}

void ApplicationHeaderComponent::set_hardware_callback(std::function<void()> callback)
{
	hardwareCallback = std::move(callback);
}

void ApplicationHeaderComponent::set_traffic_callback(std::function<void()> callback)
{
	trafficCallback = std::move(callback);
}

void ApplicationHeaderComponent::set_status(bool isRunning,
                                            const juce::String &driverName,
                                            std::size_t workingSetCount,
                                            bool isTrafficVisible)
{
	if ((running == isRunning) &&
	    (selectedDriver == driverName) &&
	    (activeWorkingSets == workingSetCount) &&
	    (trafficVisible == isTrafficVisible))
	{
		return;
	}

	running = isRunning;
	selectedDriver = driverName;
	activeWorkingSets = workingSetCount;
	trafficVisible = isTrafficVisible;
	startStopButton.setButtonText(running ? "STOP VT" : "START VT");
	startStopButton.setColour(juce::TextButton::buttonColourId, running ? AppTheme::danger() : AppTheme::success());
	startStopButton.setColour(juce::TextButton::textColourOffId, AppTheme::canvas());
	hardwareButton.setEnabled(!running);
	trafficButton.setToggleState(trafficVisible, juce::dontSendNotification);
	trafficButton.setColour(juce::TextButton::buttonColourId, trafficVisible ? AppTheme::accent() : AppTheme::surfaceRaised());
	trafficButton.setColour(juce::TextButton::textColourOffId, trafficVisible ? AppTheme::canvas() : AppTheme::text());
	repaint();
}
