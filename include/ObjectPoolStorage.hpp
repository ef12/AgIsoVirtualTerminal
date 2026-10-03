//================================================================================================
/// @file ObjectPoolStorage.hpp
///
/// @brief Stores the object pools of a working set under their version labels in a folder.
/// @details Each stored object pool component is a file object_pool_with_label_<n>.iopx that holds
/// the 7 byte version label followed by the component, with a copy of the component in
/// object_pool_<n>.iop for the log packages. Each new component gets a higher n than all
/// files in the folder, so n is the order the components were stored in.
/// @author Ehud Frank
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef OBJECT_POOL_STORAGE_HPP
#define OBJECT_POOL_STORAGE_HPP

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

class ObjectPoolStorage
{
public:
	static constexpr std::size_t VERSION_LABEL_LENGTH = 7;

	/// @brief Returns the version labels stored in a folder, the most recently stored first.
	/// @details Working sets may compare only the first few labels, KIX ones compare 4,
	/// so the version a working set stored last has to come first.
	static std::vector<std::array<std::uint8_t, VERSION_LABEL_LENGTH>> get_versions(const std::filesystem::path &folder);

	/// @brief Returns the components stored under a version label, in the order they were stored
	static std::vector<std::uint8_t> load_version(const std::filesystem::path &folder, const std::vector<std::uint8_t> &versionLabel);

	/// @brief Stores one object pool component under a version label
	static bool save_version(const std::filesystem::path &folder, const std::vector<std::uint8_t> &objectPool, const std::vector<std::uint8_t> &versionLabel);

	/// @brief Deletes the components stored under a version label
	/// @returns true if the label was stored and all its components were deleted
	static bool delete_version(const std::filesystem::path &folder, const std::vector<std::uint8_t> &versionLabel);

	/// @brief Deletes all stored versions
	/// @returns true if all stored components were deleted
	static bool delete_all_versions(const std::filesystem::path &folder);

private:
	struct StoredComponent
	{
		std::filesystem::path file;
		std::array<std::uint8_t, VERSION_LABEL_LENGTH> versionLabel = {};
		std::uint64_t index = 0;
		bool hasIndex = false;
	};

	static std::vector<StoredComponent> list_components(const std::filesystem::path &folder);
	static bool remove_component(const std::filesystem::path &folder, const StoredComponent &component);
	static bool parse_index(const std::filesystem::path &file, const std::string &prefix, std::uint64_t &index);
	static bool label_matches(const StoredComponent &component, const std::vector<std::uint8_t> &versionLabel);
};

#endif // OBJECT_POOL_STORAGE_HPP
