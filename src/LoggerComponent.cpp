//================================================================================================
/// @file LoggerComponent.cpp
///
/// @brief Implements a GUI component to draw log output.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "LoggerComponent.hpp"

#include "ServerMainComponent.hpp"

#include "Main.hpp"
#include "ModernLookAndFeel.hpp"

LoggerComponent::LoggerComponent() :
  FileLogger(File(ServerMainComponent::getAppDataDir() + "/AgISOVirtualTerminalLog.txt"),
             "Starting " + AgISOVirtualTerminalApplication::getApplicationNameWithBuildInfo(),
             1024000)
{
	setOpaque(true);
	auto bounds = getLocalBounds();
	setBounds(10, 10, bounds.getWidth() - 10, bounds.getHeight() - 10);

	startPos = getLogFile().getSize();
}

LoggerComponent::~LoggerComponent()
{
	cancelPendingUpdate();
}

void LoggerComponent::paint(Graphics &g)
{
	constexpr int headerHeight = 28;
	g.fillAll(AppTheme::surface());
	g.setColour(AppTheme::border().withAlpha(0.8f));
	g.drawRect(getLocalBounds(), 1);
	g.setColour(AppTheme::surfaceRaised());
	g.fillRect(1, 1, getWidth() - 2, headerHeight - 1);
	g.setColour(AppTheme::accent());
	g.fillRoundedRectangle(8.0f, 9.0f, 3.0f, 10.0f, 1.5f);
	g.setColour(AppTheme::text());
	g.setFont(Font(12.0f, Font::bold));
	g.drawFittedText("SYSTEM LOG", 17, 0, 110, headerHeight, Justification::centredLeft, 1);
	g.setColour(AppTheme::textMuted());
	g.setFont(11.0f);
	g.drawFittedText("Newest events first", 126, 0, getWidth() - 138, headerHeight, Justification::centredRight, 1);
	g.setFont(13.0f);

	constexpr int lineHeight = 18;
	int numberOfLinesFitted = juce::jmax(0, (getHeight() - headerHeight) / lineHeight);

	for (std::size_t i = 0; i < static_cast<int>(loggedMessages.size()) && i < numberOfLinesFitted; i++)
	{
		const auto &message = loggedMessages.at(i);
		const auto rowBounds = Rectangle<int>(0, headerHeight + (static_cast<int>(i) * lineHeight), getWidth(), lineHeight);
		if (0 == (i % 2))
		{
			g.setColour(AppTheme::surfaceRaised().withAlpha(0.38f));
			g.fillRect(rowBounds);
		}
		Colour messageColour = AppTheme::text();

		switch (message.logLevel)
		{
			case LoggingLevel::Info:
			{
				messageColour = AppTheme::text();
			}
			break;

			case LoggingLevel::Warning:
			{
				messageColour = AppTheme::warning();
			}
			break;

			case LoggingLevel::Error:
			case LoggingLevel::Critical:
			{
				messageColour = AppTheme::danger();
			}
			break;

			case LoggingLevel::Debug:
			{
				messageColour = AppTheme::transmit();
			}
			break;

			default:
			{
				messageColour = AppTheme::text();
			}
			break;
		}
		g.setColour(messageColour.withAlpha(0.55f));
		g.fillRoundedRectangle(5.0f, static_cast<float>(rowBounds.getY() + 5), 3.0f, 8.0f, 1.5f);
		g.setColour(messageColour);
		g.drawFittedText(message.logText, 14, rowBounds.getY(), getWidth() - 20, lineHeight, Justification::centredLeft, 1);
	}
}

void LoggerComponent::sink_CAN_stack_log(LoggingLevel level, const std::string &logText)
{
	// CAN stack callbacks run on worker threads. Waiting for JUCE's message
	// manager lock here can deadlock shutdown: the message thread waits for the
	// CAN update thread to stop while that update thread waits for the message
	// thread. Queue the visual update and let the message thread consume it.
	logMessage(logText);
	{
		const ScopedLock lock(pendingMessagesLock);
		pendingMessages.push_back({ logText, level });
		if (pendingMessages.size() > MAX_NUMBER_MESSAGES)
		{
			pendingMessages.pop_front();
		}
	}
	triggerAsyncUpdate();
}

void LoggerComponent::handleAsyncUpdate()
{
	std::deque<LogData> messagesToDisplay;
	{
		const ScopedLock lock(pendingMessagesLock);
		messagesToDisplay.swap(pendingMessages);
	}

	for (auto &message : messagesToDisplay)
	{
		loggedMessages.push_front(std::move(message));
	}

	while (loggedMessages.size() > MAX_NUMBER_MESSAGES)
	{
		loggedMessages.pop_back();
	}

	const auto bounds = getLocalBounds();
	const int requiredHeight = 28 + (static_cast<int>(loggedMessages.size()) * 18);
	setSize(bounds.getWidth(), juce::jmax(requiredHeight, getHeight()));
	repaint();
}

std::uint64_t LoggerComponent::initialPos() const
{
	return startPos;
}
