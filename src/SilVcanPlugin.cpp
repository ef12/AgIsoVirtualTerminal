/*******************************************************************************
** @file       SilVcanPlugin.cpp
** @brief      AgIsoStack++ CAN hardware plugin wrapping the SIL virtual CAN
*******************************************************************************/
#include "SilVcanPlugin.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

SilVcanPlugin::SilVcanPlugin(std::uint16_t localPort,
                             const std::string &remoteIp,
                             std::uint16_t remotePort,
                             std::uint32_t timeoutMs) :
  remoteIp(remoteIp),
  localPort(localPort),
  remotePort(remotePort),
  timeoutMs(timeoutMs)
{
}

SilVcanPlugin::~SilVcanPlugin()
{
	close();
}

std::string SilVcanPlugin::get_name() const
{
	return "SIL vCAN (UDP " + std::to_string(localPort) + " <-> " + remoteIp + ":" + std::to_string(remotePort) + ")";
}

bool SilVcanPlugin::get_is_valid() const
{
	return silConfig.initialized;
}

void SilVcanPlugin::open()
{
	if (silConfig.initialized)
	{
		return;
	}

	SilVcanConfigParams params{};
	params.local_port = localPort;
	params.remote_ip = remoteIp.c_str();
	params.remote_port = remotePort;
	params.timeout_ms = timeoutMs;
	params.max_pending_tx = 2000;
	params.max_rx_queue = 2000;

	if (sil_vcan_config_init(&silConfig, &params))
	{
		isobus::CANStackLogger::info("[SIL vCAN]: Initialized on UDP " + std::to_string(localPort) + " <-> " + remoteIp + ":" + std::to_string(remotePort));
	}
	else
	{
		isobus::CANStackLogger::error("[SIL vCAN]: Failed to initialize on UDP " + std::to_string(localPort));
	}
}

void SilVcanPlugin::close()
{
	if (silConfig.initialized)
	{
		sil_vcan_config_deinit(&silConfig);
	}
}

bool SilVcanPlugin::read_frame(isobus::CANMessageFrame &canFrame)
{
	if (!silConfig.initialized)
	{
		return false;
	}

	CanFrame frame{};
	CanDriver *driver = sil_vcan_config_get_driver(&silConfig);

	if (nullptr == driver)
	{
		return false;
	}

	if (!can_driver_receive(driver, &frame))
	{
		return false;
	}

	canFrame.identifier = frame.id;
	canFrame.dataLength = frame.dlc;
	canFrame.isExtendedFrame = (frame.id > 0x7FFU);
	canFrame.timestamp_us = 0;
	canFrame.channel = 0;

	for (std::uint8_t i = 0; i < frame.dlc && i < 8; i++)
	{
		canFrame.data[i] = frame.data[i];
	}

	return true;
}

bool SilVcanPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
{
	if (!silConfig.initialized)
	{
		return false;
	}

	CanFrame frame{};
	frame.id = canFrame.identifier;
	frame.dlc = canFrame.dataLength;

	for (std::uint8_t i = 0; i < canFrame.dataLength && i < 8; i++)
	{
		frame.data[i] = canFrame.data[i];
	}

	CanDriver *driver = sil_vcan_config_get_driver(&silConfig);

	if (nullptr == driver)
	{
		return false;
	}

	return can_driver_send(driver, &frame);
}
