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
	virtual asset::ptr import(const filesystem::path& pPath, const filesystem::path& mRoot_path) = 0;

protected:
	// Helper
	static filesystem::path make_path_relative(const filesystem::path& pPath, const filesystem::path& mRoot_path)
	{
		filesystem::path result(pPath);
		result.erase(result.begin(), result.begin() + mRoot_path.size());
		return result;
	}
};

class asset_manager :
	public core::system
{
	WGE_SYSTEM("Asset Manager", 0);
public:

	// TODO: Implement the filesystem_interface as the only means of
	//   loading assets.
	void set_filesystem(filesystem::filesystem_interface* pFilesystem)
	{
		mFilesystem = pFilesystem;
	}

	void add_asset(asset::ptr pAsset)
	{
		for (const auto& i : mAsset_list)
			if (i->get_id() == pAsset->get_id())
				return; // Already exists
		mAsset_list.push_back(pAsset);
	}

	asset::ptr find_asset(const filesystem::path& pPath) const
	{
		for (const auto& i : mAsset_list)
			if (i->get_path() == pPath)
				return i;
		return{};
	}
	asset::ptr find_asset(asset_uid pUID) const
	{
		for (const auto& i : mAsset_list)
			if (i->get_id() == pUID)
				return i;
		return{};
	}

	template <typename T>
	asset::tptr<T> get_asset(const filesystem::path& pPath) const
	{
		return cast_asset<T>(find_asset(pPath));
	}

	template <typename T>
	asset::tptr<T> get_asset(asset_uid pUID) const
	{
		return cast_asset<T>(find_asset(pUID));
	}

	void add_loader(const std::string& pType, asset_loader* pLoader)
	{
		assert(mLoader_list.find(pType) == mLoader_list.end());
		mLoader_list[pType] = pLoader;
	}

	bool import(const filesystem::path& pPath)
	{
		filesystem::path absolute_path = system_fs::absolute(pPath);
		for (auto& i : mLoader_list)
		{
			if (i.second->can_import(absolute_path))
			{
				asset::ptr nasset = i.second->import(absolute_path, mRoot_dir);
				add_asset(nasset);
				std::cout << "Imported " << nasset->get_path().string() << "\n";
				return true;
			}
		}
		return false;
	}

	void set_root_directory(const filesystem::path& pPath)
	{
		mRoot_dir = system_fs::absolute(pPath);
	}

	void load_assets()
	{
		filesystem::path absolute_root_path = system_fs::absolute(mRoot_dir);
		auto absolute_paths = get_absolute_path_list(absolute_root_path);
		auto relative_paths = convert_to_relative_paths(absolute_paths, system_fs::absolute(mRoot_dir));
		for (std::size_t i = 0; i < absolute_paths.size(); i++)
		{
			if (absolute_paths[i].extension() == ".asset")
			{

			}
		}
	}

private:
	// Iterates through all the files in the directory and returns a list of
	// paths to files with the extesion ".asset"
	static std::vector<filesystem::path> get_absolute_path_list(const filesystem::path& pPath)
	{
		std::vector<filesystem::path> result;
		for (const auto& i : system_fs::recursive_directory_iterator(pPath.to_system_path()))
			if (!system_fs::is_directory(i.path()) && i.path().extension() == ".asset")
				result.emplace_back(system_fs::absolute(i.path()));
		return result;
	}

	static std::vector<filesystem::path> convert_to_relative_paths(const std::vector<filesystem::path>& pList, const filesystem::path& pAbsolute_root)
	{
		std::vector<filesystem::path> result(pList.begin(), pList.end());
		for (auto& i : result)
		{
			// Erase everything at and below the root directory "dir/root/dir/file" -> "dir/file"
			i.erase(i.begin(), i.begin() + pAbsolute_root.size());
			std::cout << "Asset found: " << i.string() << "\n";
		}
		return result;
	}

private:
	std::vector<asset::ptr> mAsset_list;
	std::map<std::string, asset_loader*> mLoader_list;
	filesystem::path mRoot_dir;
	filesystem::filesystem_interface* mFilesystem;
};

}