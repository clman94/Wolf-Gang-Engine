#include <fstream>

#include <wge/core/asset_manager.hpp>
#include <wge/logging/log.hpp>
#include <wge/filesystem/file_input_stream.hpp>
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

std::string asset_manager::get_unique_name(const filesystem::path& pPath) const
{
	int index = 0;
	filesystem::path new_path = pPath;
	while (has_asset(new_path))
		new_path = pPath.parent() / (pPath.filename() + std::to_string(++index));
	return new_path.filename();
}

asset::ptr asset_manager::create_folder(const filesystem::path& pPath)
{
	// Create the asset
	auto folder = std::make_shared<asset>();
	folder->set_name(get_unique_name(pPath));
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
	store_asset(folder);
	return folder;
}

std::string asset_manager::generate_asset_directory_name(const asset::ptr& pAsset) const
{
	assert(pAsset);
	auto path = get_asset_path(pAsset);
	return path.string('.');
}

primary_asset_location::ptr asset_manager::create_asset_storage(const core::asset::ptr& pAsset) const
{
	auto directory = mRoot_dir / generate_asset_directory_name(pAsset);
	log::info("Creating new asset storage location at '{}'", directory.string());
	system_fs::create_directory(directory);
	return primary_asset_location::create(directory, pAsset->get_name());
}

asset::ptr asset_manager::load_asset(const filesystem::path& pPath)
{
	assert(pPath.extension() == ".wga");
	auto location = primary_asset_location::create(pPath.parent(), pPath.stem());

	try {
		// Read the file
		std::string str;
		{
			std::ifstream stream(pPath.string().c_str());
			if (!stream)
			{
				log::error("Could not open file '{}'", pPath.string());
				return nullptr;
			}
			str = std::string{ std::istreambuf_iterator<char>(stream), {} };
		}
		return deserialize_asset(json::parse(str), location);
	}
	catch (const json::exception& e)
	{
		log::error("In {}", pPath.string());
		log::error("Error parsing asset configuration");
		log::error("{}", e.what());
	}
	catch (...)
	{
		log::error("In {}", pPath.string());
		log::error("Unknown error while parsing asset configuration");
	}
	return nullptr;
}

void asset_manager::serialize_asset(const core::asset::ptr& pAsset, json& pJson) const
{
	pAsset->serialize(pJson);
	pJson["secondary_assets"] = json::array();
	for_each_child(pAsset, [&](const core::asset::ptr& pChild)
	{
		if (pChild->is_secondary_asset())
		{
			json j;
			serialize_asset(pChild, j);
			pJson["secondary_assets"].push_back(std::move(j));
		}
	});
}

asset::ptr asset_manager::deserialize_asset(const json& pJson, const asset_location::ptr& pLocation)
{
	asset::ptr new_asset = std::make_shared<asset>();
	new_asset->set_location(pLocation);
	new_asset->deserialize(pJson);
	if (auto res = create_resource_for(new_asset))
		res->load();
	if (pJson.count("secondary_assets") != 0)
	{
		for (auto& i : pJson["secondary_assets"])
		{
			std::string name = i["name"].get<std::string>();
			deserialize_asset(i,
				secondary_asset_location::create(
					std::dynamic_pointer_cast<primary_asset_location>(pLocation), name));
		}
	}
	add_asset(new_asset);
	return new_asset;
}

asset::ptr asset_manager::create_primary_asset(const filesystem::path& pPath, const std::string& pType)
{
	log::info("Creating new asset at path '{}' with type '{}'", pPath.string(), pType);

	// Setup some of the config for the asset.
	auto new_asset = std::make_shared<asset>();
	new_asset->set_name(get_unique_name(pPath));
	if (new_asset->get_name() != pPath.filename())
		log::info("Conflicting name '{}', renaming to '{}'", pPath.filename(), new_asset->get_name());

	new_asset->set_type(pType);
	if (auto parent_asset = get_asset(pPath.parent()))
		new_asset->set_parent(parent_asset);
	// Generate a new storage location.
	store_asset(new_asset);
	// Create the resource object.
	// Note: The resource is not loaded yet. Gotta make this function
	//   useful for imports.
	create_resource_for(new_asset);
	// Save it for good measure.
	save_asset(new_asset);
	return new_asset;
}

asset::ptr asset_manager::create_secondary_asset(const asset::ptr& pParent, const std::string& pName, const std::string& pType, const asset_id& pCustom_id)
{
	// Parent is required.
	assert(pParent);
	assert(pParent->get_location());

	log::info("Creating new secondary asset at path '{}' with type '{}'", (get_asset_path(pParent) / pName).string(), pType);

	// Setup some of the config for the asset.
	auto new_asset = std::make_shared<asset>();
	if (pCustom_id.is_valid())
		new_asset->set_id(pCustom_id);
	new_asset->set_name(get_unique_name(get_asset_path(pParent) / pName));
	if (new_asset->get_name() != pName)
		log::info("Conflicting name '{}', renaming to '{}'", pName, new_asset->get_name());

	new_asset->set_type(pType);
	new_asset->set_parent(pParent);
	new_asset->set_location(
		secondary_asset_location::create(
			std::dynamic_pointer_cast<primary_asset_location>(pParent->get_location()),
			pName));
	// Because we aren't using store_asset in this, we must add the asset manually.
	add_asset(new_asset);
	// Create the resource object.
	// Note: The resource is not loaded yet. Gotta make this function
	//   useful for imports.
	create_resource_for(new_asset);
	// Save it for good measure.
	save_asset(new_asset);
	return new_asset;
}

void asset_manager::save_asset(const core::asset::ptr& pAsset) const
{
	assert(pAsset);
	if (auto resource = pAsset->get_resource())
	{
		resource->save();
	}
	if (pAsset->is_primary_asset())
	{
		json j;
		serialize_asset(pAsset, j);
		filesystem::file_stream out;
		out.open(pAsset->get_location()->get_autonamed_file(".wga"), filesystem::stream_access::write);
		out.write(j.dump(2));
	}
	else if (pAsset->get_parent_id().is_valid())
	{
		// Secondary assets are stored in the parent asset so
		// the parent asset must be saved instead.
		save_asset(get_asset(pAsset->get_parent_id()));
	}
}

void asset_manager::store_asset(const core::asset::ptr& pAsset)
{
	auto location = create_asset_storage(pAsset);
	pAsset->set_location(location);
	save_asset(pAsset);
	add_asset(pAsset);
}

bool asset_manager::rename_asset(const core::asset::ptr& pAsset, const std::string& pTo)
{
	if (has_asset(get_asset_path(pAsset).parent() / pTo) || !pAsset->is_primary_asset() || pAsset->get_name() == pTo)
		return false;
	pAsset->set_name(pTo);
	update_directory_structure();
	save_asset(pAsset);
	return true;
}

bool asset_manager::move_asset(const core::asset::ptr& pAsset, const core::asset::ptr& pTo)
{
	if (has_asset(get_asset_path(pTo) / pAsset->get_name()) || // No name conflicts.
		!pAsset->is_primary_asset() || // Needs to be primary.

		(pTo != nullptr && // If its not root then...
			(has_parent(pTo->get_id(), pAsset->get_id()) || // it can't move into its own sub-directory.
			pAsset->get_id() == pTo->get_id()))) // it can't move into itself.
		return false;
	pAsset->set_parent(pTo);
	update_directory_structure();
	save_asset(pAsset);
	return true;
}

void asset_manager::remove_asset_storage(const core::asset::ptr& pAsset) const
{
	if (pAsset->is_primary_asset())
	{
		auto dir_path = pAsset->get_location()->get_directory();
		assert(!dir_path.empty());

		if (system_fs::exists(dir_path))
		{
			log::info("Removing folder {}", dir_path.string());

			// Remove the directory.
			system_fs::remove_all(dir_path);
		}
	}
}

void asset_manager::update_directory_structure()
{
	for (const auto& i : mAsset_list)
	{
		// Only primary assets can be moved.
		auto primary_location = std::dynamic_pointer_cast<primary_asset_location>(i->get_location());
		if (!primary_location)
			continue;

		std::string new_dir_name = generate_asset_directory_name(i);

		// Get the path of the asset definition's parent directory.
		auto current_dir_path = i->get_location()->get_directory();
		assert(!current_dir_path.empty());

		// Make sure the name is consistant with the generated name.
		if (current_dir_path.filename() != new_dir_name)
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

			auto parent_dir_path = current_dir_path.parent();
			assert(!parent_dir_path.empty());
			auto new_dir_path = parent_dir_path / new_dir_name;

			// Move the directory and update the autonamed files.
			primary_location->move_to(new_dir_path, i->get_name());

			// Reload the asset if it was loaded before.
			if (res && !res->is_loaded() && was_resource_loaded)
				res->load();

			save_asset(i);
		}
	}
}

void asset_manager::save_all_configuration()
{
	for (auto& i : mAsset_list)
		save_asset(i);
}

bool asset_manager::has_parent(const asset_id& pTop, const asset_id& pParent) const
{
	core::asset::ptr i = get_asset(pTop);
	do {
		i = get_asset(i->get_parent_id());
	} while (i != nullptr && i->get_id() != pParent);
	return i != nullptr;
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

asset::ptr asset_manager::get_asset(const asset_id& pUID) const noexcept
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

bool asset_manager::has_asset(const asset_id& pUID) const noexcept
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

bool asset_manager::is_valid_path(const filesystem::path& pPath) const noexcept
{
	// Yes, empty paths are valid because they refer to the "root"
	if (pPath.empty())
		return true;
	// Everything else needs to be a valid asset.
	return has_asset(pPath);
}

bool asset_manager::remove_asset(const asset::ptr& pAsset)
{
	if (!pAsset)
		return false;

	// Remove all children assets.
	auto all_children = get_children_recursive(pAsset);
	for (const auto& i : all_children)
		remove_asset(i);

	remove_asset_storage(pAsset);

	log::info("Removing asset \"{}\" from registry", get_asset_path(pAsset).string());

	// Remove it from the asset manager.
	auto iter = std::remove(mAsset_list.begin(), mAsset_list.end(), pAsset);
	assert(iter != mAsset_list.end());
	mAsset_list.erase(iter);
	
	return true;
}

filesystem::path asset_manager::get_asset_path(const core::asset::ptr& pAsset) const
{
	if (!pAsset)
		return{};
	filesystem::path result;
	for (core::asset::ptr i = pAsset; i; i = get_asset(i->get_parent_id()))
		result.push_front(i->get_name());
	return result;
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
	filesystem::path absolute_root_path = mRoot_dir;
	auto absolute_paths_list = get_absolute_path_list(absolute_root_path);
	for (const auto& i : absolute_paths_list)
	{
		if (i.extension() == ".wga")
		{
			if (!load_asset(i))
			{
				log::warning("Skipping asset \"{}\" due to an error", i.string());
			}
		}
	}
}

const asset_manager::asset_container& asset_manager::get_asset_list() const
{
	return mAsset_list;
}

} // namespace wge::core
