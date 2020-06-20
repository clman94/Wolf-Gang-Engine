#pragma once

#include <wge/filesystem/path.hpp>

#include <memory>

namespace wge::core
{

class asset_location
{
public:
	using ptr = std::shared_ptr<asset_location>;

	static ptr create(const filesystem::path& pDirectory, const std::string& pName)
	{
		return std::make_shared<asset_location>(pDirectory, pName);
	}

	asset_location(const filesystem::path& pDirectory, const std::string& pName) :
		mDirectory(pDirectory),
		mName(pName)
	{}
	asset_location(asset_location&&) = default;
	asset_location(const asset_location&) = delete;

	filesystem::path get_autonamed_file(const std::string& pSuffix) const
	{
		assert(is_valid());
		assert(!pSuffix.empty());
		return mDirectory / (mName + pSuffix);
	}

	filesystem::path get_file(const std::string& pFilename) const
	{
		assert(is_valid());
		assert(!pFilename.empty());
		return mDirectory / pFilename;
	}

	bool remove_autonamed_file(const std::string& pExtension)
	{
		assert(is_valid());
		assert(!pExtension.empty());
		system_fs::remove(get_autonamed_file(pExtension));
	}

	bool remove_file(const std::string& pFilename)
	{
		assert(is_valid());
		assert(!pFilename.empty());
		system_fs::remove(get_file(pFilename));
	}

	// Moves the directory and renames the autonamed files.
	void move_to(const filesystem::path& pDirectory, const std::string& pName)
	{
		assert(is_valid());
		assert(!pDirectory.empty());
		assert(!pName.empty());

		// Move the directory.
		system_fs::rename(mDirectory, pDirectory);
		mDirectory = pDirectory;

		// Get all autonamed files with the old name.
		std::vector<filesystem::path> autonamed_files;
		for (auto i : system_fs::directory_iterator(pDirectory))
		{
			if (i.path().stem() == mName)
				autonamed_files.push_back(i.path());
		}

		// Rename those autonamed files with the new name.
		mName = pName;
		for (auto& i : autonamed_files)
			system_fs::rename(i, get_autonamed_file(i.extension()));
	}

	const std::string& get_name() const noexcept
	{
		assert(is_valid());
		return mName;
	}

	const filesystem::path& get_directory() const noexcept
	{
		assert(is_valid());
		return mDirectory;
	}

	bool is_valid() const noexcept
	{
		return !mName.empty() && !mDirectory.empty();
	}

private:
	std::string mName;
	filesystem::path mDirectory;
};

} // namespace wge::core
