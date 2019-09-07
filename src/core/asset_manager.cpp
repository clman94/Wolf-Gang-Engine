#include <fstream>

#include <wge/core/asset_manager.hpp>
#include <wge/logging/log.hpp>
#include <filesystem>

namespace wge::core
{

bool asset_manager::has_children(const asset::ptr& pParent) const
{
	for (const auto& i : mAsset_list)
		if (i && i->get_parent_id() == pParent->get_id())
			return true;
	return false;
}

bool asset_manager::has_subfolders(const asset::ptr& pParent) const
{
	for (const auto& i : mAsset_list)
		if (i && i->get_parent_id() == pParent->get_id()
			&& i->get_type() == "folder")
			return true;
	return false;
}

std::vector<asset::ptr> asset_manager::get_children(const asset::ptr& pParent) const
{
	std::vector<asset::ptr> result;
	for_each_child(pParent, [&result](const asset::ptr& pChild)
	{
		result.push_back(pChild);
	});
	return result;
}

std::vector<asset::ptr> asset_manager::get_children_recursive(const asset::ptr& pParent) const
{
	std::vector<asset::ptr> result;
	for_each_child_recursive(pParent, [&result](const asset::ptr& pChild)
	{
		result.push_back(pChild);
	});
	return result;
}

asset::ptr asset_manager::find_child(const asset::ptr& pParent, const std::string_view& pName) const
{
	if (pParent)
	{
		// Find all assets with this parent.
		for (const auto& i : mAsset_list)
			if (i && i->get_parent_id() == pParent->get_id() &&
				i->get_name() == pName)
				return i;
	}
	else
	{
		// Asset is in root directory.
		for (const auto& i : mAsset_list)
			if (i && !i->get_parent_id().is_valid()
				&& i->get_name() == pName)
				return i;
	}
	return{};
}

asset::ptr asset_manager::create_folder(const filesystem::path& pPath)
{
	// Check if there is already an asset with this name.
	if (has_asset(pPath))
		return{};

	// Create the asset
	auto folder = std::make_shared<asset>();
	folder->set_name(pPath.filename());
	folder->set_type("folder");

	// This path specify a parent directory?
	if (pPath.size() > 1)
	{
		filesystem::path parent_path = pPath;
		parent_path.pop_filepath();

		asset::ptr parent = get_asset(parent_path);

		// Create the parent folder if it doesn't exist
		if (!parent)
			parent = create_folder(parent_path);

		// Sanity
		assert(parent);

		// Link to this directory.
		folder->set_parent(parent);
	}

	auto system_dir = mRoot_dir / generate_asset_directory_name(folder);
	system_fs::create_directory(system_dir);
	folder->set_file_path(system_dir / (folder->get_name() + ".wga"));
	folder->save();

	add_asset(folder);

	return folder;
}

filesystem::path asset_manager::create_asset_storage(const asset::ptr& pAsset)
{
	auto dir = mRoot_dir / pAsset->get_id().to_string();
	system_fs::create_directory(dir);
	return dir;
}

std::string asset_manager::generate_asset_directory_name(const asset::ptr& pAsset) const
{
	assert(pAsset);
	auto path = get_asset_path(pAsset);
	return path.string('.') + "[" + pAsset->get_id().to_shortened_string() + "]";
}

filesystem::path asset_manager::create_asset_storage(const core::asset::ptr& pAsset) const
{
	auto directory = mRoot_dir / generate_asset_directory_name(pAsset);
	system_fs::create_directory(directory);
	return directory;
}

void asset_manager::store_asset(const core::asset::ptr& pAsset) const
{
	auto directory = mRoot_dir / generate_asset_directory_name(pAsset);
	pAsset->set_file_path(directory / (pAsset->get_name() + ".wga"));
}

void asset_manager::update_directory_structure()
{
	for (const auto& i : mAsset_list)
	{
		std::string dir_name = generate_asset_directory_name(i);

		// Get the path of the asset definition's parent directory.
		auto current_dir_path = i->get_file_path().parent();
		assert(!current_dir_path.empty());

		// Make sure the name is consistant with the generated name.
		if (current_dir_path.filename() != dir_name)
		{
			// It is not! So we must rename it.

			resource* res = i->get_resource();

			bool was_resource_loaded = false;

			// Make sure the resource is unloaded to get rid of
			// those pesky streams.
			if (res)
			{
				was_resource_loaded = res->is_loaded();
				res->unload();
			}

			{
				// Rename the asset's configuration file just-in-case.
				auto new_config_path = current_dir_path / (i->get_name() + ".wga");
				system_fs::rename(i->get_file_path(), new_config_path);
			}

			auto parent_dir_path = current_dir_path.parent();
			assert(!parent_dir_path.empty());

			auto new_dir_path = parent_dir_path / dir_name;

			// Rename the directory with the correct name.
			system_fs::rename(current_dir_path, new_dir_path);
			
			if (res)
			{
				// Tell the resource that its source path has changed.
				res->update_source_path(new_dir_path, i->get_name());

				// Reload the asset if it was loaded before.
				if (!res->is_loaded() && was_resource_loaded)
					res->load(new_dir_path, i->get_name());
			}

			// Tell the asset configuration that its source path has changed.
			i->set_file_path(new_dir_path / (i->get_name() + ".wga"));
		}
	}
}

void asset_manager::save_all_configuration()
{
	for (auto& i : mAsset_list)
		i->save();
}

filesystem::path asset_manager::make_relative_to_root(const filesystem::path& pPath) const
{
	filesystem::path path = pPath;
	path.erase(path.begin(), path.begin() + mRoot_dir.size());
	return path;
}


static std::vector<filesystem::path> get_absolute_path_list(const filesystem::path& pPath)
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
	}
}

asset::ptr asset_manager::get_asset(const filesystem::path& pPath) const noexcept
{
	asset::ptr ptr;
	for (auto& i : pPath)
	{
		ptr = find_child(ptr, i);
		if (!ptr)
			return{};
	}
	return ptr;
}

asset::ptr asset_manager::get_asset(const util::uuid& pUID) const noexcept
{
	if (!pUID.is_valid())
		return{};
	for (const auto& i : mAsset_list)
		if (i->get_id() == pUID)
			return i;
	return{};
}

bool asset_manager::has_asset(const filesystem::path& pPath) const noexcept
{
	return (bool)get_asset(pPath);
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
	if (!pAsset)
		return false;
	for (auto& i : mAsset_list)
		if (pAsset == i)
			return true;
	return false;
}

bool asset_manager::remove_asset(const asset::ptr& pAsset)
{
	if (!pAsset)
		return false;

	// Remove all children assets.
	auto all_children = get_children_recursive(pAsset);
	for (const auto& i : all_children)
		remove_asset(i);

	// Get the path to the asset's parent directory.
	auto dir_path = pAsset->get_file_path().parent();
	assert(!dir_path.empty());

	// Remove the directory.
	bool success = system_fs::remove_all(dir_path);

	// Remove it from the asset manager.
	auto iter = std::remove(mAsset_list.begin(), mAsset_list.end(), pAsset);
	assert(iter != mAsset_list.end());
	mAsset_list.erase(iter);

	return success;
}

filesystem::path asset_manager::get_asset_path(const core::asset::ptr& pAsset) const
{
	filesystem::path result;
	for (core::asset::ptr i = pAsset; i; i = get_asset(i->get_parent_id()))
		result.push_front(i->get_name());
	return result;
}

void asset_manager::register_resource_factory(const std::string& pType, const resource_factory& pFactory)
{
	mResource_factories[pType] = pFactory;
}

resource::uptr asset_manager::create_resource(const std::string& pType) const
{
	auto iter = mResource_factories.find(pType);
	if (iter != mResource_factories.end())
		return iter->second();
	return{};
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
	filesystem::path absolute_root_path = mRoot_dir;
	auto absolute_paths_list = get_absolute_path_list(absolute_root_path);
	for (const auto& i : absolute_paths_list)
	{
		if (i.extension() == ".wga")
		{
			// Create and load the new asset.
			auto ptr = std::make_shared<asset>();
			if (!ptr->load_file(i))
			{
				log::error() << "Failed to parse asset configuration for asset at \"" << i.string() << "\"" << log::endm;
				continue;
			}

			// Create the resource if it can.
			auto factory_iter = mResource_factories.find(ptr->get_type());
			if (auto res = create_resource(ptr->get_type()))
			{
				res->load(ptr->get_file_path().parent(), ptr->get_name());
				ptr->set_resource(std::move(res));
			}

			add_asset(ptr);
		}
	}
}

const asset_manager::asset_container& asset_manager::get_asset_list() const
{
	return mAsset_list;
}

static bool is_already_imported(const filesystem::path& pResource_path)
{
	filesystem::path path = pResource_path;
	auto filename = path.pop_filepath();
	path.push_back(filename + ".wge_asset");
	return system_fs::exists(path);
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
	if (has_asset(pPath))
	{
		log::error() << "Could not create asset " << std::quoted(pPath.string()) << "; it already exists" << log::endm;
		return{};
	}
	auto config = std::make_shared<asset>();
	config->set_file_path(mRoot_dir / append_asset_extension(pPath));
	config->set_name(pPath.filename());
	config->set_type(pType);
	config->set_metadata(pMetadata);
	config->save();

	add_asset(config);
	return config;
}


} // namespace wge::core
