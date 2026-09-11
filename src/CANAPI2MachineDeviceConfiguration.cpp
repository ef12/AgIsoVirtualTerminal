//================================================================================================
/// @file CANAPI2MachineDeviceConfiguration.cpp
///
/// @brief Implements Windows machine-wide configuration for the PEAK CAN-API 2 default device.
/// @author The Open-Agriculture Developers
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================

#include "CANAPI2MachineDeviceConfiguration.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <shellapi.h>

#include <array>
#include <sstream>
#include <vector>

namespace
{
	constexpr const char *CAN_API2_REGISTRY_KEY = "SOFTWARE\\PEAK-System\\CanApi2";
	constexpr const char *CAN_API2_DEVICE_VALUE = "Device";
	constexpr const char *HELPER_ARGUMENT_PREFIX = "--set-canapi2-machine-device=";
	constexpr int HELPER_EXIT_WRITE_32_FAILED = 10;
	constexpr int HELPER_EXIT_WRITE_64_FAILED = 11;
	constexpr int HELPER_EXIT_VERIFY_FAILED = 12;

	struct RegistryValue
	{
		bool available = false;
		std::string value;
		LONG error = ERROR_SUCCESS;
	};

	RegistryValue read_registry_value(REGSAM registryView)
	{
		RegistryValue result;
		HKEY key = nullptr;
		result.error = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
		                             CAN_API2_REGISTRY_KEY,
		                             0,
		                             KEY_QUERY_VALUE | registryView,
		                             &key);
		if (ERROR_SUCCESS != result.error)
		{
			return result;
		}

		DWORD type = 0;
		DWORD size = 0;
		result.error = RegQueryValueExA(key,
		                                CAN_API2_DEVICE_VALUE,
		                                nullptr,
		                                &type,
		                                nullptr,
		                                &size);
		if ((ERROR_SUCCESS != result.error) ||
		    ((REG_SZ != type) && (REG_EXPAND_SZ != type)))
		{
			if (ERROR_SUCCESS == result.error)
			{
				result.error = ERROR_DATATYPE_MISMATCH;
			}
			RegCloseKey(key);
			return result;
		}

		std::vector<char> buffer(size + 1, '\0');
		result.error = RegQueryValueExA(key,
		                                CAN_API2_DEVICE_VALUE,
		                                nullptr,
		                                &type,
		                                reinterpret_cast<LPBYTE>(buffer.data()),
		                                &size);
		RegCloseKey(key);

		if (ERROR_SUCCESS == result.error)
		{
			result.available = true;
			result.value.assign(buffer.data());
		}
		return result;
	}

	LONG write_registry_value(REGSAM registryView, const char *deviceName)
	{
		HKEY key = nullptr;
		DWORD disposition = 0;
		LONG result = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
		                              CAN_API2_REGISTRY_KEY,
		                              0,
		                              nullptr,
		                              REG_OPTION_NON_VOLATILE,
		                              KEY_SET_VALUE | registryView,
		                              nullptr,
		                              &key,
		                              &disposition);
		if (ERROR_SUCCESS == result)
		{
			const DWORD valueSize = static_cast<DWORD>(std::char_traits<char>::length(deviceName) + 1);
			result = RegSetValueExA(key,
			                        CAN_API2_DEVICE_VALUE,
			                        0,
			                        REG_SZ,
			                        reinterpret_cast<const BYTE *>(deviceName),
			                        valueSize);
			RegCloseKey(key);
		}
		return result;
	}

	bool registry_values_match(const char *expectedDevice)
	{
		const auto value32 = read_registry_value(KEY_WOW64_32KEY);
		const auto value64 = read_registry_value(KEY_WOW64_64KEY);
		return value32.available &&
		  value64.available &&
		  (expectedDevice == value32.value) &&
		  (expectedDevice == value64.value);
	}

	std::string format_windows_error(DWORD error)
	{
		LPSTR rawMessage = nullptr;
		const DWORD length = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		                                      FORMAT_MESSAGE_FROM_SYSTEM |
		                                      FORMAT_MESSAGE_IGNORE_INSERTS,
		                                    nullptr,
		                                    error,
		                                    0,
		                                    reinterpret_cast<LPSTR>(&rawMessage),
		                                    0,
		                                    nullptr);
		std::string result;
		if ((0 != length) && (nullptr != rawMessage))
		{
			result.assign(rawMessage, length);
			while ((!result.empty()) &&
			       ((result.back() == '\r') || (result.back() == '\n') || (result.back() == ' ')))
			{
				result.pop_back();
			}
		}
		if (nullptr != rawMessage)
		{
			LocalFree(rawMessage);
		}
		return result;
	}

	std::string describe_registry_value(const RegistryValue &value)
	{
		if (value.available)
		{
			return value.value;
		}
		if (ERROR_FILE_NOT_FOUND == value.error)
		{
			return "not configured";
		}
		return "unavailable (Windows error " + std::to_string(value.error) + ")";
	}

	CANAPI2MachineDeviceConfiguration::UpdateResult run_elevated_helper(CANAPI2MachineDeviceConfiguration::Device device)
	{
		std::array<wchar_t, 32768> executablePath{};
		const DWORD pathLength = GetModuleFileNameW(nullptr,
		                                            executablePath.data(),
		                                            static_cast<DWORD>(executablePath.size()));
		if ((0 == pathLength) || (pathLength >= executablePath.size()))
		{
			const DWORD error = GetLastError();
			return { CANAPI2MachineDeviceConfiguration::UpdateStatus::Failed,
				       "Could not locate the Virtual Terminal executable (Windows error " +
				         std::to_string(error) + ": " + format_windows_error(error) + ")." };
		}

		const std::string argument = std::string(HELPER_ARGUMENT_PREFIX) +
		  CANAPI2MachineDeviceConfiguration::get_device_name(device);
		const std::wstring wideArgument(argument.begin(), argument.end());

		SHELLEXECUTEINFOW executeInfo{};
		executeInfo.cbSize = sizeof(executeInfo);
		executeInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		executeInfo.lpVerb = L"runas";
		executeInfo.lpFile = executablePath.data();
		executeInfo.lpParameters = wideArgument.c_str();
		executeInfo.nShow = SW_HIDE;

		if (FALSE == ShellExecuteExW(&executeInfo))
		{
			const DWORD error = GetLastError();
			if (ERROR_CANCELLED == error)
			{
				return { CANAPI2MachineDeviceConfiguration::UpdateStatus::UserCancelled,
					       "Administrator permission was cancelled. The machine-wide PCAN device was not changed." };
			}
			return { CANAPI2MachineDeviceConfiguration::UpdateStatus::Failed,
				       "Could not start the elevated PCAN configuration helper (Windows error " +
				         std::to_string(error) + ": " + format_windows_error(error) + ")." };
		}

		const DWORD waitResult = WaitForSingleObject(executeInfo.hProcess, INFINITE);
		DWORD exitCode = static_cast<DWORD>(-1);
		const bool exitCodeAvailable = (WAIT_OBJECT_0 == waitResult) &&
		  (FALSE != GetExitCodeProcess(executeInfo.hProcess, &exitCode));
		CloseHandle(executeInfo.hProcess);

		if ((!exitCodeAvailable) ||
		    (CANAPI2MachineDeviceConfiguration::HELPER_EXIT_SUCCESS != static_cast<int>(exitCode)))
		{
			std::ostringstream message;
			message << "The elevated helper could not update both CAN-API 2 registry views";
			if (exitCodeAvailable)
			{
				message << " (exit code " << exitCode << ")";
			}
			message << ". The VT selection was not changed.";
			return { CANAPI2MachineDeviceConfiguration::UpdateStatus::Failed, message.str() };
		}

		return { CANAPI2MachineDeviceConfiguration::UpdateStatus::Updated, {} };
	}
}

namespace CANAPI2MachineDeviceConfiguration
{
	bool try_parse_helper_argument(const std::string &argument, Device &device)
	{
		const std::string prefix(HELPER_ARGUMENT_PREFIX);
		if (0 != argument.compare(0, prefix.size(), prefix))
		{
			return false;
		}

		const auto requestedDevice = argument.substr(prefix.size());
		if ("pcan_usb" == requestedDevice)
		{
			device = Device::USB;
			return true;
		}
		if ("pcan_virtual" == requestedDevice)
		{
			device = Device::Virtual;
			return true;
		}
		return false;
	}

	int set_machine_default_device(Device device)
	{
		const char *deviceName = get_device_name(device);
		if (ERROR_SUCCESS != write_registry_value(KEY_WOW64_32KEY, deviceName))
		{
			return HELPER_EXIT_WRITE_32_FAILED;
		}
		if (ERROR_SUCCESS != write_registry_value(KEY_WOW64_64KEY, deviceName))
		{
			return HELPER_EXIT_WRITE_64_FAILED;
		}
		if (!registry_values_match(deviceName))
		{
			return HELPER_EXIT_VERIFY_FAILED;
		}
		return HELPER_EXIT_SUCCESS;
	}

	UpdateResult ensure_machine_default_device(Device device)
	{
		const char *deviceName = get_device_name(device);
		if (registry_values_match(deviceName))
		{
			return { UpdateStatus::AlreadyConfigured, {} };
		}

		auto result = run_elevated_helper(device);
		if ((UpdateStatus::Updated == result.status) &&
		    (!registry_values_match(deviceName)))
		{
			result.status = UpdateStatus::Failed;
			result.message = "The elevated helper completed, but the 32-bit and 64-bit CAN-API 2 settings could not be verified.";
		}
		return result;
	}

	std::string get_machine_default_summary()
	{
		const auto value32 = read_registry_value(KEY_WOW64_32KEY);
		const auto value64 = read_registry_value(KEY_WOW64_64KEY);
		if (value32.available && value64.available && (value32.value == value64.value))
		{
			return value32.value + " (32-bit and 64-bit)";
		}
		return "32-bit: " + describe_registry_value(value32) + ", 64-bit: " + describe_registry_value(value64);
	}

	const char *get_device_name(Device device)
	{
		return (Device::Virtual == device) ? "pcan_virtual" : "pcan_usb";
	}
}
