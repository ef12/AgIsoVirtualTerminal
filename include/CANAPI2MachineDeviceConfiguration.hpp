//================================================================================================
/// @file CANAPI2MachineDeviceConfiguration.hpp
///
/// @brief Provides Windows machine-wide configuration for the PEAK CAN-API 2 default device.
/// @author The Open-Agriculture Developers
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef CAN_API2_MACHINE_DEVICE_CONFIGURATION_HPP
#define CAN_API2_MACHINE_DEVICE_CONFIGURATION_HPP

#include <string>

namespace CANAPI2MachineDeviceConfiguration
{
	/// @brief A PEAK CAN-API 2 device which can be selected as the machine default.
	enum class Device
	{
		USB,
		Virtual
	};

	/// @brief Result of requesting a machine-wide device change.
	enum class UpdateStatus
	{
		AlreadyConfigured,
		Updated,
		UserCancelled,
		Failed
	};

	/// @brief Detailed result returned to the GUI after a machine-wide device change.
	struct UpdateResult
	{
		UpdateStatus status = UpdateStatus::Failed;
		std::string message;
	};

	/// @brief Exit code returned by the elevated helper when the registry update succeeded.
	constexpr int HELPER_EXIT_SUCCESS = 0;

	/// @brief Parses the private command-line argument used by the elevated helper instance.
	/// @param[in] argument A complete command-line argument.
	/// @param[out] device The requested device when the argument is valid.
	/// @returns true when the argument is a valid helper request.
	bool try_parse_helper_argument(const std::string &argument, Device &device);

	/// @brief Writes and verifies the CAN-API 2 default in both Windows registry views.
	/// @attention This function must run in an elevated process.
	/// @param[in] device Device to configure as the machine default.
	/// @returns A helper process exit code. Zero indicates success.
	int set_machine_default_device(Device device);

	/// @brief Ensures both Windows registry views use the requested CAN-API 2 default device.
	/// @details Starts an elevated helper instance when a registry update is required.
	/// @param[in] device Device to configure as the machine default.
	/// @returns The result and a user-facing diagnostic message when applicable.
	UpdateResult ensure_machine_default_device(Device device);

	/// @brief Returns a human-readable summary of the 32-bit and 64-bit machine defaults.
	std::string get_machine_default_summary();

	/// @brief Returns the CAN-API 2 driver name for a device.
	const char *get_device_name(Device device);
}

#endif // CAN_API2_MACHINE_DEVICE_CONFIGURATION_HPP
