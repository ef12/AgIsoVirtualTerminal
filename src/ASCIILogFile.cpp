/*******************************************************************************
** @file       ASCIILogFile.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ASCIILogFile.hpp"

#include "isobus/utility/to_string.hpp"

#include <cmath>

ASCIILogFile::ASCIILogFile() = default;

ASCIILogFile::~ASCIILogFile()
{
	stop();
}

bool ASCIILogFile::start(const File &targetFile)
{
	stop();

	auto selectedFile = targetFile;
	if (!selectedFile.hasFileExtension(".asc"))
	{
		selectedFile = selectedFile.withFileExtension(".asc");
	}

	if (!selectedFile.getParentDirectory().createDirectory())
	{
		return false;
	}

	if (selectedFile.existsAsFile() && !selectedFile.deleteFile())
	{
		return false;
	}

	auto newOutputStream = selectedFile.createOutputStream();
	if ((nullptr == newOutputStream) || !newOutputStream->openedOk())
	{
		return false;
	}

	const auto currentTime = Time::getCurrentTime().toString(true, true, true, false);
	newOutputStream->writeText("date " + currentTime + "\n", false, false, nullptr);
	newOutputStream->writeText("base hex timestamps absolute\n", false, false, nullptr);
	newOutputStream->writeText("no internal events logged\n", false, false, nullptr);
	newOutputStream->flush();

	{
		const std::lock_guard<std::mutex> lock(outputMutex);
		logFile = selectedFile;
		outputStream = std::move(newOutputStream);
		initialTimestampMilliseconds = Time::getMillisecondCounterHiRes();
	}

	recording.store(true);
	canFrameReceivedListener = isobus::CANHardwareInterface::get_can_frame_received_event_dispatcher().add_listener([this](const isobus::CANMessageFrame &canFrame) {
		write_frame(canFrame, false);
	});
	canFrameSentListener = isobus::CANHardwareInterface::get_can_frame_transmitted_event_dispatcher().add_listener([this](const isobus::CANMessageFrame &canFrame) {
		write_frame(canFrame, true);
	});
	return true;
}

void ASCIILogFile::stop()
{
	recording.store(false);

	if (0 != canFrameReceivedListener)
	{
		isobus::CANHardwareInterface::get_can_frame_received_event_dispatcher().remove_listener(canFrameReceivedListener);
		canFrameReceivedListener = 0;
	}
	if (0 != canFrameSentListener)
	{
		isobus::CANHardwareInterface::get_can_frame_transmitted_event_dispatcher().remove_listener(canFrameSentListener);
		canFrameSentListener = 0;
	}

	const std::lock_guard<std::mutex> lock(outputMutex);
	if (outputStream)
	{
		outputStream->flush();
		outputStream.reset();
	}
}

bool ASCIILogFile::is_recording() const
{
	return recording.load();
}

File ASCIILogFile::current_log_file() const
{
	const std::lock_guard<std::mutex> lock(outputMutex);
	return logFile;
}

void ASCIILogFile::write_frame(const isobus::CANMessageFrame &canFrame, bool transmitted)
{
	const std::lock_guard<std::mutex> lock(outputMutex);
	if (!recording.load() || !outputStream)
	{
		return;
	}

	const auto elapsedMilliseconds = Time::getMillisecondCounterHiRes() - initialTimestampMilliseconds;
	auto milliseconds = isobus::to_string(static_cast<std::uint64_t>(elapsedMilliseconds) % 1000);
	while (milliseconds.length() < 3)
	{
		milliseconds = "0" + milliseconds;
	}

	String line = "   " + String(static_cast<std::uint64_t>(std::floor(elapsedMilliseconds / 1000.0))) +
	  "." + milliseconds + "000 " + String(static_cast<int>(canFrame.channel) + 1) + "  " +
	  String::toHexString(canFrame.identifier).toUpperCase() + (canFrame.isExtendedFrame ? "x" : " ") +
	  "       " + (transmitted ? "Tx" : "Rx") + "   d " + String(static_cast<int>(canFrame.dataLength)) + " ";

	for (std::uint_fast8_t index = 0; index < canFrame.dataLength; ++index)
	{
		line += String::toHexString(canFrame.data[index]).paddedLeft('0', 2).toUpperCase() + " ";
	}
	line += "\n";

	outputStream->writeText(line, false, false, nullptr);
}
