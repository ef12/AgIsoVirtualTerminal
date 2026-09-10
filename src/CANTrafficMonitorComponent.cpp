//================================================================================================
/// @file CANTrafficMonitorComponent.cpp
///
/// @brief Implements the live CAN traffic table.
/// @author The Open-Agriculture Developers
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include "CANTrafficMonitorComponent.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>

namespace
{
	enum ColumnIds
	{
		Time = 1,
		Direction,
		Channel,
		FrameType,
		Identifier,
		DataLength,
		Data
	};
}

CANTrafficMonitorComponent::CANTrafficMonitorComponent() :
  table("CAN Traffic", this),
  clearButton("Clear"),
  pauseButton("Pause"),
  autoScrollButton("Auto-scroll"),
  dockButton("Undock")
{
	initialTimestampMilliseconds = Time::getMillisecondCounterHiRes();

	titleLabel.setText("CAN Traffic", dontSendNotification);
	titleLabel.setFont(Font(15.0f, Font::bold));
	titleLabel.setJustificationType(Justification::centredLeft);
	statusLabel.setJustificationType(Justification::centredLeft);

	auto &header = table.getHeader();
	header.addColumn("Time (s)", ColumnIds::Time, 90, 70, 140);
	header.addColumn("Dir", ColumnIds::Direction, 46, 40, 70);
	header.addColumn("Ch", ColumnIds::Channel, 42, 35, 60);
	header.addColumn("Type", ColumnIds::FrameType, 52, 45, 70);
	header.addColumn("Identifier", ColumnIds::Identifier, 100, 80, 150);
	header.addColumn("DLC", ColumnIds::DataLength, 45, 40, 60);
	header.addColumn("Data", ColumnIds::Data, 300, 140, 1000);
	header.setStretchToFitActive(true);
	table.setRowHeight(22);
	table.setHeaderHeight(24);
	table.setColour(ListBox::backgroundColourId, Colour(0xff17191c));
	table.setColour(ListBox::outlineColourId, Colour(0xff4b4e52));
	table.setOutlineThickness(1);

	autoScrollButton.setToggleState(true, dontSendNotification);
	pauseButton.setClickingTogglesState(true);
	pauseButton.onClick = [this]() {
		paused.store(pauseButton.getToggleState());
		pauseButton.setButtonText(paused.load() ? "Resume" : "Pause");
		update_status_label();
	};
	clearButton.onClick = [this]() {
		clear_frames();
	};
	dockButton.onClick = [this]() {
		if (dockToggleCallback)
		{
			dockToggleCallback();
		}
	};

	addAndMakeVisible(titleLabel);
	addAndMakeVisible(statusLabel);
	addAndMakeVisible(clearButton);
	addAndMakeVisible(pauseButton);
	addAndMakeVisible(autoScrollButton);
	addAndMakeVisible(dockButton);
	addAndMakeVisible(table);
	update_status_label();

	receivedFrameListener = isobus::CANHardwareInterface::get_can_frame_received_event_dispatcher().add_listener([this](const isobus::CANMessageFrame &frame) {
		queue_frame(frame, Direction::Receive);
	});
	transmittedFrameListener = isobus::CANHardwareInterface::get_can_frame_transmitted_event_dispatcher().add_listener([this](const isobus::CANMessageFrame &frame) {
		queue_frame(frame, Direction::Transmit);
	});
}

CANTrafficMonitorComponent::~CANTrafficMonitorComponent()
{
	isobus::CANHardwareInterface::get_can_frame_received_event_dispatcher().remove_listener(receivedFrameListener);
	isobus::CANHardwareInterface::get_can_frame_transmitted_event_dispatcher().remove_listener(transmittedFrameListener);
	cancelPendingUpdate();
	table.setModel(nullptr);
}

void CANTrafficMonitorComponent::paint(Graphics &graphics)
{
	graphics.fillAll(Colour(0xff25282c));
}

void CANTrafficMonitorComponent::resized()
{
	auto bounds = getLocalBounds().reduced(6);
	auto toolbar = bounds.removeFromTop(30);
	dockButton.setBounds(toolbar.removeFromRight(78).reduced(2));
	clearButton.setBounds(toolbar.removeFromRight(62).reduced(2));
	pauseButton.setBounds(toolbar.removeFromRight(72).reduced(2));
	autoScrollButton.setBounds(toolbar.removeFromRight(100).reduced(2));
	titleLabel.setBounds(toolbar.removeFromLeft(105));
	statusLabel.setBounds(toolbar);
	bounds.removeFromTop(4);
	table.setBounds(bounds);
}

void CANTrafficMonitorComponent::set_dock_toggle_callback(std::function<void()> callback)
{
	dockToggleCallback = std::move(callback);
}

void CANTrafficMonitorComponent::set_is_docked(bool isDocked)
{
	dockButton.setButtonText(isDocked ? "Undock" : "Dock");
}

void CANTrafficMonitorComponent::set_capture_enabled(bool enabled)
{
	captureEnabled.store(enabled);
}

int CANTrafficMonitorComponent::getNumRows()
{
	return static_cast<int>(displayedFrames.size());
}

void CANTrafficMonitorComponent::paintRowBackground(Graphics &graphics, int rowNumber, int, int, bool rowIsSelected)
{
	if (rowIsSelected)
	{
		graphics.fillAll(Colour(0xff365d7d));
	}
	else if ((rowNumber >= 0) && (rowNumber < static_cast<int>(displayedFrames.size())))
	{
		const auto &frame = displayedFrames.at(static_cast<std::size_t>(rowNumber));
		const Colour baseColour = (Direction::Receive == frame.direction) ? Colour(0xff17241d) : Colour(0xff18212a);
		graphics.fillAll((0 == (rowNumber % 2)) ? baseColour : baseColour.brighter(0.08f));
	}
}

void CANTrafficMonitorComponent::paintCell(Graphics &graphics, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
	if ((rowNumber < 0) || (rowNumber >= static_cast<int>(displayedFrames.size())))
	{
		return;
	}

	const auto &frame = displayedFrames.at(static_cast<std::size_t>(rowNumber));
	String text;
	switch (columnId)
	{
		case ColumnIds::Time:
			text = String(frame.timestampSeconds, 6);
			break;

		case ColumnIds::Direction:
			text = (Direction::Receive == frame.direction) ? "Rx" : "Tx";
			break;

		case ColumnIds::Channel:
			text = String(static_cast<int>(frame.channel));
			break;

		case ColumnIds::FrameType:
			text = frame.isExtendedFrame ? "EXT" : "STD";
			break;

		case ColumnIds::Identifier:
			text = "0x" + String::toHexString(static_cast<int>(frame.identifier)).paddedLeft('0', frame.isExtendedFrame ? 8 : 3).toUpperCase();
			break;

		case ColumnIds::DataLength:
			text = String(static_cast<int>(frame.dataLength));
			break;

		case ColumnIds::Data:
			for (std::uint8_t index = 0; index < frame.dataLength; ++index)
			{
				if (index > 0)
				{
					text += " ";
				}
				text += String::toHexString(static_cast<int>(frame.data[index])).paddedLeft('0', 2).toUpperCase();
			}
			break;

		default:
			break;
	}

	if (rowIsSelected)
	{
		graphics.setColour(Colours::white);
	}
	else
	{
		graphics.setColour((Direction::Receive == frame.direction) ? Colour(0xff9ee6b8) : Colour(0xff9dcdf5));
	}
	graphics.setFont(13.0f);
	graphics.drawText(text, 4, 0, width - 8, height, Justification::centredLeft, true);
}

void CANTrafficMonitorComponent::handleAsyncUpdate()
{
	std::deque<FrameRecord> framesToAdd;
	{
		const std::lock_guard<std::mutex> lock(pendingFramesMutex);
		framesToAdd.swap(pendingFrames);
	}

	for (auto &frame : framesToAdd)
	{
		if (Direction::Receive == frame.direction)
		{
			++receivedFrameCount;
		}
		else
		{
			++transmittedFrameCount;
		}
		displayedFrames.push_back(std::move(frame));
	}
	while (displayedFrames.size() > MAX_DISPLAYED_FRAMES)
	{
		displayedFrames.pop_front();
	}

	table.updateContent();
	if (autoScrollButton.getToggleState() && !displayedFrames.empty())
	{
		table.scrollToEnsureRowIsOnscreen(static_cast<int>(displayedFrames.size() - 1));
	}
	update_status_label();
	table.repaint();
}

void CANTrafficMonitorComponent::queue_frame(const isobus::CANMessageFrame &frame, Direction direction)
{
	if (!captureEnabled.load() || paused.load())
	{
		return;
	}

	FrameRecord record;
	record.identifier = frame.identifier;
	record.dataLength = static_cast<std::uint8_t>(std::min<std::size_t>(frame.dataLength, record.data.size()));
	record.channel = frame.channel;
	record.direction = direction;
	record.isExtendedFrame = frame.isExtendedFrame;
	std::copy_n(frame.data, record.dataLength, record.data.begin());

	{
		const std::lock_guard<std::mutex> lock(pendingFramesMutex);
		record.timestampSeconds = (Time::getMillisecondCounterHiRes() - initialTimestampMilliseconds) / 1000.0;
		if (pendingFrames.size() >= MAX_PENDING_FRAMES)
		{
			pendingFrames.pop_front();
			droppedFrameCount.fetch_add(1);
		}
		pendingFrames.push_back(std::move(record));
	}
	triggerAsyncUpdate();
}

void CANTrafficMonitorComponent::clear_frames()
{
	{
		const std::lock_guard<std::mutex> lock(pendingFramesMutex);
		pendingFrames.clear();
		initialTimestampMilliseconds = Time::getMillisecondCounterHiRes();
	}
	displayedFrames.clear();
	receivedFrameCount = 0;
	transmittedFrameCount = 0;
	droppedFrameCount.store(0);
	table.updateContent();
	update_status_label();
	table.repaint();
}

void CANTrafficMonitorComponent::update_status_label()
{
	String status = String(displayedFrames.size()) + " shown | Rx " + String(receivedFrameCount) + " | Tx " + String(transmittedFrameCount);
	const auto dropped = droppedFrameCount.load();
	if (dropped > 0)
	{
		status += " | Dropped " + String(dropped);
	}
	if (paused.load())
	{
		status += " | Paused";
	}
	statusLabel.setText(status, dontSendNotification);
}
