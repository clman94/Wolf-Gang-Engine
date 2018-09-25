#pragma once

#include <wge/core/system.hpp>
#include <wge/core/asset.hpp>
#include <wge/filesystem/filesystem_interface.hpp>

#include <vector>
#include <map>
#include <iostream>

namespace wge::core
{

class asset_loader
{
public:
	// Create an asset from and config file
	virtual asset::ptr create_asset(asset_config::ptr pConfig, const filesystem::path& mRoot_path) = 0;

	// Return true if this loader can import the file at this path.
	// pPath will be an absolute path to the file.
	virtual bool can_import(const filesystem::path& pPath) = 0;

	// Import a resource file as an asset.
	// pPath will be an absolute path to the file.
	virtual asset::ptr import_asset(const filesystem::path& pPath, const filesystem::path& mRoot_path) = 0;

protected:
	// Helper
	static filesystem::path make_path_relative(const filesystem::path& pPath, const filesystem::path& mRoot_path);
};

class asset_manager :
	public core::system
{
	WGE_SYSTEM("Asset Manager", 0);
public:
	// TODO: Implement the filesystem_interface as the only means of
	//   loading assets.
	void set_filesystem(filesystem::filesystem_interface* pFilesystem);

	// Manually add an asset
	void add_asset(asset::ptr pAsset);

	// Find an asset by its relative path.
	// Returns empty if it is not found.
	asset::ptr find_asset(const filesystem::path& pPath) const;
	// Find an asset by its uid.
	// Returns empty when it it not found.
	asset::ptr find_asset(asset_uid pUID) const;

	// Find an asset by its relative path.
	// Returns empty if it is not found.
	// It will automatically be casted to T.
	template <typename T>
	asset::tptr<T> get_asset(const filesystem::path& pPath) const
	{
		return cast_asset<T>(find_asset(pPath));
	}
	// Find an asset by its uid.
	// Returns empty if it is not found.
	// It will automatically be casted to T.
	template <typename T>
	asset::tptr<T> get_asset(asset_uid pUID) const
	{
		return cast_asset<T>(find_asset(pUID));
	}

	// Add a loader for a specific type of asset
	void add_loader(const std::string& pType, asset_loader* pLoader);

	// Import a file as an asset
	bool import_asset(const filesystem::path& pPath);

	// Set the root directory to find all assets.
	// Note: This affects the relative path of all assets.
	void set_root_directory(const filesystem::path& pPath);

	// Load all assets in the root directory
	void load_assets();

private:
	// Iterates through all the files in the directory and returns a list of
	// paths to files with the extesion ".asset"
	static std::vector<filesystem::path> get_absolute_path_list(const filesystem::path& pPath);

	asset_loader* find_loader(const std::string& pType) const;

private:
	std::vector<asset::ptr> mAsset_list;
	std::map<std::string, asset_loader*> mLoader_list;
	filesystem::path mRoot_dir;
	filesystem::filesystem_interface* mFilesystem;
};

}