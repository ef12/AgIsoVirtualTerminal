//================================================================================================
/// @file ASCIILogFile.hpp
///
/// @brief Defines a CAN logger that saves messages in a Vector .asc file.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef ASCII_LOG_FILE_HPP
#define ASCII_LOG_FILE_HPP

#include "isobus/hardware_integration/can_hardware_interface.hpp"

#include "JuceHeader.h"

#include <atomic>
#include <memory>
#include <mutex>

/// @brief Logs to Vector .asc file
class ASCIILogFile
{
public:
	ASCIILogFile();

	~ASCIILogFile();

	/// @brief Starts a new Vector ASCII CAN recording at the selected path.
	bool start(const File &targetFile);

	/// @brief Stops the active recording and flushes it to disk.
	void stop();

	bool is_recording() const;
	File current_log_file() const;

private:
	void write_frame(const isobus::CANMessageFrame &canFrame, bool transmitted);

	File logFile;
	std::unique_ptr<FileOutputStream> outputStream;
	isobus::EventCallbackHandle canFrameReceivedListener = 0;
	isobus::EventCallbackHandle canFrameSentListener = 0;
	std::atomic_bool recording{ false };
	double initialTimestampMilliseconds = 0.0;
	mutable std::mutex outputMutex;
};

#endif // ASCII_LOG_FILE_HPP
