#include <fstream>

#include <wge/core/asset_manager.hpp>
#include <wge/logging/log.hpp>
#include <filesystem>

namespace wge::core
{

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

// Convert a resource filepath into a wgemetadata filepath
inline filesystem::path make_metadata_config_path(const filesystem::path & pPath)
{
	filesystem::path config_path(pPath);
	config_path.pop_filepath();
	config_path /= pPath.filename() + ".wgemetadata";
	return config_path;
}

// Create metadata configuration from a resource
inline core::asset_config::ptr create_metadata_config(const std::string & pType, const filesystem::path & pPath)
{
	core::asset_config::ptr config = std::make_shared<core::asset_config>();
	config->set_type(pType);
	config->set_path(make_metadata_config_path(pPath));
	config->set_id(util::generate_uuid());
	config->save();

	return config;
}

// Load a config/metadata file
inline asset_config::ptr load_asset_config(const filesystem::path & pPath)
{
	asset_config::ptr config = std::make_shared<asset_config>();
	config->set_path(pPath);

	// Parse configuration file
	std::ifstream stream(pPath.string().c_str());
	std::string config_str(std::istreambuf_iterator<char>(stream), {});
	config->deserialize(json::parse(config_str));
	return config;
}

void asset_manager::set_filesystem(filesystem::filesystem_interface * pFilesystem)
{
	mFilesystem = pFilesystem;
}

void asset_manager::register_asset(const std::string& pType, const asset_factory& pFactory)
{
	mAsset_factories[pType] = pFactory;
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

asset::ptr asset_manager::find_asset(const filesystem::path& pPath) const noexcept
{
	for (const auto& i : mAsset_list)
		if (i->get_path() == pPath)
			return i;
	return{};
}

asset::ptr asset_manager::find_asset(const util::uuid& pUID) const noexcept
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

void asset_manager::register_config_extension(const std::string& pType, const std::string& pExtension)
{
	mAsset_config_extensions[pExtension] = pType;
}

void asset_manager::register_resource_extension(const std::string& pType, const std::string& pExtension)
{
	mAsset_resource_extensions[pExtension] = pType;
}

void asset_manager::register_serial_config_extension(const std::string& pType, const std::string& pExtension)
{
	register_asset<serialized_asset>(pType);
	register_config_extension(pType, pExtension);
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
		// Ignore metadata config files
		if (i.extension() == ".wgemetadata")
			continue;

		auto config_iter = mAsset_config_extensions.find(i.extension());
		auto res_iter = mAsset_resource_extensions.find(i.extension());

		if (config_iter != mAsset_config_extensions.end())
		{
			load_configuration_asset(i, config_iter->second);
		}
		else if (res_iter != mAsset_resource_extensions.end())
		{
			load_resource_asset(i, res_iter->second);
		}
		else
		{
			log::warning() << "Unknown asset file type " << std::filesystem::relative(i) << ", ignoring" << log::endm;
		}
	}
}

const asset_manager::asset_container& asset_manager::get_asset_list() const
{
	return mAsset_list;
}

asset::ptr asset_manager::create_configuration_asset(const std::string& pType, const filesystem::path& pPath)
{
	assert(mAsset_factories.find(pType) != mAsset_factories.end());

	auto config = std::make_shared<asset_config>();
	config->set_path(mRoot_dir / pPath);
	config->set_type(pType);
	config->set_id(util::generate_uuid());

	auto nasset = mAsset_factories[pType](pPath, config);
	add_asset(nasset);
	return nasset;
}

filesystem::path asset_manager::make_relative_to_root(const filesystem::path & pPath) const
{
	filesystem::path path = pPath;
	path.erase(path.begin(), path.begin() + mRoot_dir.size());
	return path;
}

void asset_manager::load_configuration_asset(const filesystem::path& pPath, const std::string& pType)
{
	add_asset(mAsset_factories[pType](make_relative_to_root(pPath), load_asset_config(pPath)));
}

void asset_manager::load_resource_asset(const filesystem::path& pPath, const std::string& pType)
{
	asset_config::ptr config;
	filesystem::path config_path = make_metadata_config_path(pPath);
	if (std::filesystem::exists(config_path.to_system_path()))
	{
		config = load_asset_config(config_path);
	}
	else
	{
		config = create_metadata_config(pType, pPath);
		log::info() << "Imported asset " << make_relative_to_root(pPath).to_system_path() << log::endm;
	}

	add_asset(mAsset_factories[pType](make_relative_to_root(pPath), config));
}

} // namespace wge::core
