/*******************************************************************************
** @file       SilVcanPlugin.hpp
** @brief      AgIsoStack++ CAN hardware plugin wrapping the SIL virtual CAN
*******************************************************************************/
#pragma once

#include "isobus/hardware_integration/can_hardware_plugin.hpp"

#include <cstdint>
#include <string>

extern "C"
{
#include "sil_vcan_config.h"
}

class SilVcanPlugin : public isobus::CANHardwarePlugin
{
public:
	SilVcanPlugin(std::uint16_t localPort,
	              const std::string &remoteIp,
	              std::uint16_t remotePort,
	              std::uint32_t timeoutMs = 1);

	~SilVcanPlugin() override;

	std::string get_name() const override;
	bool get_is_valid() const override;
	void open() override;
	void close() override;
	bool read_frame(isobus::CANMessageFrame &canFrame) override;
	bool write_frame(const isobus::CANMessageFrame &canFrame) override;

private:
	SilVcanConfig silConfig{};
	std::string remoteIp;
	std::uint16_t localPort;
	std::uint16_t remotePort;
	std::uint32_t timeoutMs;
};
