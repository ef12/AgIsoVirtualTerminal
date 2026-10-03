//================================================================================================
/// @file ObjectPoolStorage.cpp
///
/// @brief Stores the object pools of a working set under their version labels in a folder.
/// @author Ehud Frank
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include "ObjectPoolStorage.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace
{
	const std::string LABELED_FILE_PREFIX = "object_pool_with_label_";
	const std::string LABELED_FILE_EXTENSION = ".iopx";
	const std::string COPY_FILE_PREFIX = "object_pool_";
	const std::string COPY_FILE_EXTENSION = ".iop";
	constexpr std::size_t MAX_INDEX_DIGITS = 18;
}

std::vector<std::array<std::uint8_t, ObjectPoolStorage::VERSION_LABEL_LENGTH>> ObjectPoolStorage::get_versions(const std::filesystem::path &folder)
{
	std::vector<std::array<std::uint8_t, VERSION_LABEL_LENGTH>> retVal;
	auto components = list_components(folder);

	// Newest first, and each label only where it was stored last
	std::reverse(components.begin(), components.end());
	for (const auto &component : components)
	{
		if (retVal.end() == std::find(retVal.begin(), retVal.end(), component.versionLabel))
		{
			retVal.push_back(component.versionLabel);
		}
	}
	return retVal;
}

std::vector<std::uint8_t> ObjectPoolStorage::load_version(const std::filesystem::path &folder, const std::vector<std::uint8_t> &versionLabel)
{
	std::vector<std::uint8_t> retVal;

	for (const auto &component : list_components(folder))
	{
		if (label_matches(component, versionLabel))
		{
			std::ifstream file(component.file, std::ios::binary | std::ios::ate);
			const std::streamoff fileSize = file.is_open() ? static_cast<std::streamoff>(file.tellg()) : 0;

			if (fileSize > static_cast<std::streamoff>(VERSION_LABEL_LENGTH))
			{
				const auto componentSize = static_cast<std::size_t>(fileSize) - VERSION_LABEL_LENGTH;
				const auto offset = retVal.size();
				retVal.resize(offset + componentSize);
				file.seekg(static_cast<std::streamoff>(VERSION_LABEL_LENGTH), std::ios::beg);
				file.read(reinterpret_cast<char *>(retVal.data() + offset), static_cast<std::streamsize>(componentSize));
				retVal.resize(offset + static_cast<std::size_t>(file.gcount()));
			}
		}
	}
	return retVal;
}

bool ObjectPoolStorage::save_version(const std::filesystem::path &folder, const std::vector<std::uint8_t> &objectPool, const std::vector<std::uint8_t> &versionLabel)
{
	std::error_code error;
	std::uint64_t nextIndex = 0;

	if (VERSION_LABEL_LENGTH != versionLabel.size())
	{
		return false;
	}

	std::filesystem::create_directories(folder, error);
	for (std::filesystem::directory_iterator entry(folder, error), end; (!error) && (entry != end); entry.increment(error))
	{
		std::uint64_t index = 0;
		if (((LABELED_FILE_EXTENSION == entry->path().extension()) && parse_index(entry->path(), LABELED_FILE_PREFIX, index)) ||
		    ((COPY_FILE_EXTENSION == entry->path().extension()) && parse_index(entry->path(), COPY_FILE_PREFIX, index)))
		{
			nextIndex = std::max(nextIndex, index + 1);
		}
	}

	const auto indexText = std::to_string(nextIndex);
	std::ofstream labeledFile(folder / (LABELED_FILE_PREFIX + indexText + LABELED_FILE_EXTENSION), std::ios::trunc | std::ios::binary);
	labeledFile.write(reinterpret_cast<const char *>(versionLabel.data()), static_cast<std::streamsize>(versionLabel.size()));
	labeledFile.write(reinterpret_cast<const char *>(objectPool.data()), static_cast<std::streamsize>(objectPool.size()));
	labeledFile.close();

	std::ofstream copyFile(folder / (COPY_FILE_PREFIX + indexText + COPY_FILE_EXTENSION), std::ios::trunc | std::ios::binary);
	copyFile.write(reinterpret_cast<const char *>(objectPool.data()), static_cast<std::streamsize>(objectPool.size()));

	return !labeledFile.fail();
}

bool ObjectPoolStorage::delete_version(const std::filesystem::path &folder, const std::vector<std::uint8_t> &versionLabel)
{
	bool versionFound = false;
	bool allRemoved = true;

	for (const auto &component : list_components(folder))
	{
		if (label_matches(component, versionLabel))
		{
			versionFound = true;
			allRemoved &= remove_component(folder, component);
		}
	}
	return versionFound && allRemoved;
}

bool ObjectPoolStorage::delete_all_versions(const std::filesystem::path &folder)
{
	bool allRemoved = true;

	for (const auto &component : list_components(folder))
	{
		allRemoved &= remove_component(folder, component);
	}
	return allRemoved;
}

std::vector<ObjectPoolStorage::StoredComponent> ObjectPoolStorage::list_components(const std::filesystem::path &folder)
{
	std::vector<StoredComponent> retVal;
	std::error_code error;

	for (std::filesystem::directory_iterator entry(folder, error), end; (!error) && (entry != end); entry.increment(error))
	{
		if (LABELED_FILE_EXTENSION == entry->path().extension())
		{
			StoredComponent component;
			std::ifstream file(entry->path(), std::ios::binary);

			component.file = entry->path();
			component.hasIndex = parse_index(entry->path(), LABELED_FILE_PREFIX, component.index);
			if (file.read(reinterpret_cast<char *>(component.versionLabel.data()), VERSION_LABEL_LENGTH))
			{
				retVal.push_back(component);
			}
		}
	}

	// The folder lists file names in text order, so object_pool_with_label_10 comes before _9
	std::stable_sort(retVal.begin(), retVal.end(), [](const StoredComponent &first, const StoredComponent &second) {
		return first.index < second.index;
	});
	return retVal;
}

bool ObjectPoolStorage::remove_component(const std::filesystem::path &folder, const StoredComponent &component)
{
	std::error_code error;
	const bool retVal = std::filesystem::remove(component.file, error);

	if (component.hasIndex)
	{
		std::filesystem::remove(folder / (COPY_FILE_PREFIX + std::to_string(component.index) + COPY_FILE_EXTENSION), error);
	}
	return retVal;
}

bool ObjectPoolStorage::parse_index(const std::filesystem::path &file, const std::string &prefix, std::uint64_t &index)
{
	const auto stem = file.stem().string();
	const bool retVal = (stem.size() > prefix.size()) &&
	  ((stem.size() - prefix.size()) <= MAX_INDEX_DIGITS) &&
	  (0 == stem.compare(0, prefix.size(), prefix)) &&
	  std::all_of(stem.begin() + static_cast<std::ptrdiff_t>(prefix.size()), stem.end(), [](char character) { return 0 != std::isdigit(static_cast<unsigned char>(character)); });

	if (retVal)
	{
		index = std::stoull(stem.substr(prefix.size()));
	}
	return retVal;
}

bool ObjectPoolStorage::label_matches(const StoredComponent &component, const std::vector<std::uint8_t> &versionLabel)
{
	return (VERSION_LABEL_LENGTH == versionLabel.size()) &&
	  std::equal(component.versionLabel.begin(), component.versionLabel.end(), versionLabel.begin());
}
