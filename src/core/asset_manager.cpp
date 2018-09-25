#include <fstream>

#include <wge/core/asset_manager.hpp>
using namespace wge;
using namespace wge::core;

filesystem::path asset_loader::make_path_relative(const filesystem::path & pPath, const filesystem::path & mRoot_path)
{
	filesystem::path result(pPath);
	result.erase(result.begin(), result.begin() + mRoot_path.size());
	return result;
}

void asset_manager::set_filesystem(filesystem::filesystem_interface * pFilesystem)
{
	mFilesystem = pFilesystem;
}

void asset_manager::add_asset(asset::ptr pAsset)
{
	assert(pAsset);
	for (const auto& i : mAsset_list)
		if (i->get_id() == pAsset->get_id())
			return; // Already exists
	mAsset_list.push_back(pAsset);
}

asset::ptr asset_manager::find_asset(const filesystem::path & pPath) const
{
	for (const auto& i : mAsset_list)
		if (i->get_path() == pPath)
			return i;
	return{};
}

asset::ptr asset_manager::find_asset(asset_uid pUID) const
{
	for (const auto& i : mAsset_list)
		if (i->get_id() == pUID)
			return i;
	return{};
}

void asset_manager::add_loader(const std::string & pType, asset_loader * pLoader)
{
	assert(mLoader_list.find(pType) == mLoader_list.end());
	mLoader_list[pType] = pLoader;
}

bool asset_manager::import_asset(const filesystem::path & pPath)
{
	filesystem::path absolute_path = system_fs::absolute(pPath);
	for (auto& i : mLoader_list)
	{
		if (i.second->can_import(absolute_path))
		{
			asset::ptr nasset = i.second->import_asset(absolute_path, mRoot_dir);
			add_asset(nasset);
			std::cout << "Imported " << nasset->get_path().string() << "\n";
			return true;
		}
	}
	return false;
}

void asset_manager::set_root_directory(const filesystem::path & pPath)
{
	mRoot_dir = system_fs::absolute(pPath);
}

void asset_manager::load_assets()
{
	filesystem::path absolute_root_path = system_fs::absolute(mRoot_dir);
	auto absolute_paths_list = get_absolute_path_list(absolute_root_path);
	for (const auto& i : absolute_paths_list)
	{
		if (i.extension() == ".asset")
		{
			asset_config::ptr config = std::make_shared<asset_config>();
			config->set_path(i);

			// Parse configuration file
			std::ifstream stream(i.string().c_str());
			std::string config_str(std::istreambuf_iterator<char>(stream), {});
			config->load(json::parse(config_str));

			// Load asset
			asset_loader* loader = find_loader(config->get_type());
			if (!loader)
			{
				std::cout << "Unable to load asset " << i.string() << "\n";
				continue;
			}
			add_asset(loader->create_asset(config, mRoot_dir));
		}
	}
}

std::vector<filesystem::path> asset_manager::get_absolute_path_list(const filesystem::path & pPath)
{
	std::vector<filesystem::path> result;
	for (const auto& i : system_fs::recursive_directory_iterator(pPath.to_system_path()))
		if (!system_fs::is_directory(i.path()) && i.path().extension() == ".asset")
			result.emplace_back(system_fs::absolute(i.path()));
	return result;
}

asset_loader * asset_manager::find_loader(const std::string & pType) const
{
	for (auto i : mLoader_list)
		if (i.first == pType)
			return i.second;
	return nullptr;
}
