#include <fstream>

#include <wge/core/asset_manager.hpp>
#include <wge/logging/log.hpp>
#include <filesystem>

namespace wge::core
{

inline filesystem::path asset_manager::make_relative_to_root(const filesystem::path& pPath) const
{
	filesystem::path path = pPath;
	path.erase(path.begin(), path.begin() + mRoot_dir.size());
	return path;
}

// Iterates through all the files in the directory and returns a list of
// paths to the files.
inline std::vector<filesystem::path> get_absolute_path_list(const filesystem::path & pPath)
{
	std::vector<filesystem::path> result;
	for (const auto& i : system_fs::recursive_directory_iterator(pPath.to_system_path()))
		if (!system_fs::is_directory(i.path()))
			result.emplace_back(system_fs::absolute(i.path()));
	return result;
}

void asset_manager::set_filesystem(filesystem::filesystem_interface* pFilesystem)
{
	mFilesystem = pFilesystem;
}

void asset_manager::add_asset(const asset::ptr& pAsset)
{
	assert(pAsset);
	if (!has_asset(pAsset))
	{
		mAsset_list.push_back(pAsset);
		mFile_structure.insert(pAsset->get_path(), pAsset);
	}
}

asset::ptr asset_manager::get_asset(const filesystem::path& pPath) const noexcept
{
	for (const auto& i : mAsset_list)
		if (i->get_path() == pPath)
			return i;
	return{};
}

asset::ptr asset_manager::get_asset(const util::uuid& pUID) const noexcept
{
	for (const auto& i : mAsset_list)
		if (i->get_id() == pUID)
			return i;
	return{};
}

bool asset_manager::has_asset(const util::uuid& pUID) const noexcept
{
	for (auto& i : mAsset_list)
		if (i->get_id() == pUID)
			return true;
	return false;
}

bool asset_manager::has_asset(const asset::ptr& pAsset) const noexcept
{
	for (auto& i : mAsset_list)
		if (pAsset == i)
			return true;
	return false;
}

void asset_manager::register_resource_factory(const std::string& pType, const resource_factory& pFactory)
{
	mResource_factories[pType] = pFactory;
}

void asset_manager::set_root_directory(const filesystem::path& pPath)
{
	mRoot_dir = system_fs::absolute(pPath);
}

const filesystem::path& asset_manager::get_root_directory() const
{
	return mRoot_dir;
}

void asset_manager::load_assets()
{
	filesystem::path absolute_root_path = system_fs::absolute(mRoot_dir);
	auto absolute_paths_list = get_absolute_path_list(absolute_root_path);
	for (const auto& i : absolute_paths_list)
	{
		if (i.extension() == ".wge_asset")
		{
			// Create and load the new asset.
			auto ptr = std::make_shared<asset>();
			if (!ptr->load_file(i))
			{
				log::error() << "Failed to parse asset configuration for asset at \"" << i.string() << "\"" << log::endm;
				continue;
			}

			filesystem::path path = i;
			path.remove_extension(); // Remove the .wge_asset extension
			ptr->set_path(make_relative_to_root(path));

			// Call the resource factory function if needed for this asset type.
			auto factory_iter = mResource_factories.find(ptr->get_type());
			if (factory_iter != mResource_factories.end())
				factory_iter->second(ptr);

			add_asset(ptr);
		}
	}
}

const asset_manager::asset_container& asset_manager::get_asset_list() const
{
	return mAsset_list;
}

asset::ptr asset_manager::import_resource(const filesystem::path& pResource_path, const std::string& pType)
{

	// Find the factory for the resource.
	auto factory_iter = mResource_factories.find(pType);
	if (factory_iter == mResource_factories.end())
		return{}; // Couldn't find any.

	asset::ptr config = create_asset(pResource_path, pType); // The .wge_asset extension will be added automatically.

	// Call the factory
	factory_iter->second(config);

	log::info() << "Asset Manager: Imported asset \"" << config->get_path().string() << "\"" << log::endm;

	return config;
}

static bool is_already_imported(const filesystem::path& pResource_path)
{
	filesystem::path path = pResource_path;
	auto filename = path.pop_filepath();
	path.push_back(filename + ".wge_asset");
	return system_fs::exists(path);
}

void asset_manager::import_all_with_ext(const std::string& pExtension, const std::string& pType)
{

	filesystem::path absolute_root_path = system_fs::absolute(mRoot_dir);
	auto absolute_paths_list = get_absolute_path_list(absolute_root_path);
	for (const auto& i : absolute_paths_list)
	{
		if (i.extension() == pExtension && !is_already_imported(i))
		{
			import_resource(make_relative_to_root(i), pType);
		}
	}
}

static filesystem::path append_asset_extension(const filesystem::path& pPath)
{
	filesystem::path result(pPath);
	std::string filename = result.pop_filepath();
	result.push_back(filename + ".wge_asset");
	return result;
}

asset::ptr asset_manager::create_asset(const filesystem::path& pPath, const std::string& pType, const json& pMetadata)
{
	auto config = std::make_shared<asset>();
	config->set_file_path(mRoot_dir / append_asset_extension(pPath));
	config->set_path(pPath);
	config->set_type(pType);
	config->set_metadata(pMetadata);
	config->save();

	add_asset(config);
	return config;
}


} // namespace wge::core
