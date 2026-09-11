//================================================================================================
/// @file CANTrafficMonitorComponent.hpp
///
/// @brief Defines a live CAN traffic table that can be docked in the main window.
/// @author The Open-Agriculture Developers
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef CAN_TRAFFIC_MONITOR_COMPONENT_HPP
#define CAN_TRAFFIC_MONITOR_COMPONENT_HPP

#include "ASCIILogFile.hpp"
#include "JuceHeader.h"
#include "isobus/hardware_integration/can_hardware_interface.hpp"

#include <array>
#include <atomic>
#include <deque>
#include <functional>
#include <mutex>

/// @brief Displays recently transmitted and received CAN frames.
class CANTrafficMonitorComponent : public Component
  , private TableListBoxModel
  , private AsyncUpdater
{
public:
	explicit CANTrafficMonitorComponent(ASCIILogFile &trafficLogger);
	~CANTrafficMonitorComponent() override;

	void paint(Graphics &graphics) override;
	void resized() override;

	void set_dock_toggle_callback(std::function<void()> callback);
	void set_is_docked(bool isDocked);
	void set_capture_enabled(bool enabled);
	File current_log_file() const;

	static constexpr int DOCKED_HEIGHT = 260;

private:
	enum class Direction
	{
		Receive,
		Transmit
	};

	struct FrameRecord
	{
		double timestampSeconds = 0.0;
		std::uint32_t identifier = 0;
		std::array<std::uint8_t, 8> data{};
		std::uint8_t dataLength = 0;
		std::uint8_t channel = 0;
		Direction direction = Direction::Receive;
		bool isExtendedFrame = false;
	};

	int getNumRows() override;
	void paintRowBackground(Graphics &graphics, int rowNumber, int width, int height, bool rowIsSelected) override;
	void paintCell(Graphics &graphics, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
	void handleAsyncUpdate() override;

	void queue_frame(const isobus::CANMessageFrame &frame, Direction direction);
	void clear_frames();
	void update_status_label();
	void toggle_recording();
	void choose_recording_file();

	static constexpr std::size_t MAX_DISPLAYED_FRAMES = 5000;
	static constexpr std::size_t MAX_PENDING_FRAMES = 2000;

	TableListBox table;
	Label titleLabel;
	Label statusLabel;
	TextButton clearButton;
	TextButton pauseButton;
	TextButton recordButton;
	ToggleButton autoScrollButton;
	TextButton dockButton;
	std::unique_ptr<FileChooser> recordingFileChooser;
	ASCIILogFile &trafficLogger;

	std::deque<FrameRecord> displayedFrames;
	std::deque<FrameRecord> pendingFrames;
	std::mutex pendingFramesMutex;
	std::function<void()> dockToggleCallback;
	std::atomic_bool captureEnabled{ false };
	std::atomic_bool paused{ false };
	std::atomic<std::size_t> droppedFrameCount{ 0 };
	std::size_t receivedFrameCount = 0;
	std::size_t transmittedFrameCount = 0;
	double initialTimestampMilliseconds = 0.0;
	isobus::EventCallbackHandle receivedFrameListener = 0;
	isobus::EventCallbackHandle transmittedFrameListener = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CANTrafficMonitorComponent)
};

#endif // CAN_TRAFFIC_MONITOR_COMPONENT_HPP
