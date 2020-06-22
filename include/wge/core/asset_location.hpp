#pragma once

#include <wge/filesystem/path.hpp>

#include <memory>

namespace wge::core
{

class asset_location
{
public:
	using ptr = std::shared_ptr<asset_location>;

	virtual ~asset_location() = default;

	virtual const std::string& get_name() const noexcept = 0;
	virtual const filesystem::path& get_directory() const noexcept = 0;

	filesystem::path get_autonamed_file(const std::string& pSuffix) const
	{
		assert(is_valid());
		assert(!pSuffix.empty());
		return get_directory() / (get_name() + pSuffix);
	}

	filesystem::path get_file(const std::string& pFilename) const
	{
		assert(is_valid());
		assert(!pFilename.empty());
		return get_directory() / pFilename;
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

	bool is_valid() const noexcept
	{
		return !get_name().empty() && !get_directory().empty();
	}
};

class primary_asset_location final :
	public asset_location
{
public:
	using ptr = std::shared_ptr<primary_asset_location>;

	primary_asset_location(const filesystem::path& pDirectory, const std::string& pName) :
		mDirectory(pDirectory),
		mName(pName)
	{}

	static ptr create(const filesystem::path& pDirectory, const std::string& pName)
	{
		return std::make_shared<primary_asset_location>(pDirectory, pName);
	}

	virtual const std::string& get_name() const noexcept override
	{
		return mName;
	}

	virtual const filesystem::path& get_directory() const noexcept override
	{
		return mDirectory;
	}

	// Moves the directory and renames the autonamed files.
	void move_to(const filesystem::path& pDirectory, const std::string& pName)
	{
		assert(is_valid());

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

private:
	std::string mName;
	filesystem::path mDirectory;
};

class secondary_asset_location final :
	public asset_location
{
public:
	using ptr = std::shared_ptr<secondary_asset_location>;

	secondary_asset_location(const primary_asset_location::ptr& pPrimary, const std::string& pName) :
		mPrimary(pPrimary), mName(pName)
	{
		assert(pPrimary);
		assert(!mName.empty());
		assert(pName != pPrimary->get_name());
	}

	static ptr create(const primary_asset_location::ptr& pPrimary, const std::string& pName)
	{
		return std::make_shared<secondary_asset_location>(pPrimary, pName);
	}

	virtual const std::string& get_name() const noexcept override
	{
		return mName;
	}

	virtual const filesystem::path& get_directory() const noexcept override
	{
		return mPrimary->get_directory();
	}

private:
	primary_asset_location::ptr mPrimary;
	std::string mName;
};

} // namespace wge::core
