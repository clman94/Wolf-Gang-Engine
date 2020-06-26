#pragma once

#include <wge/core/asset.hpp>
#include <wge/core/serialize_type.hpp>
#include <wge/filesystem/filesystem_interface.hpp>
#include <wge/util/ptr.hpp>

#include <vector>
#include <map>
#include <iostream>

namespace wge::core
{

class asset_manager
{
public:
	using resource_factory = std::function<void(const asset::ptr&)>;
	using asset_container = std::vector<asset::ptr>;

	// TODO: Implement the filesystem_interface as the only means of
	//   loading assets.
	void set_filesystem(filesystem::filesystem_interface* pFilesystem);

	// Manually add an asset
	void add_asset(const asset::ptr& pAsset);

	// Find an asset by its relative path.
	// Returns empty if it is not found.
	asset::ptr get_asset(const filesystem::path& pPath) const noexcept;
	// Find an asset by its uid.
	// Returns empty when it it not found.
	asset::ptr get_asset(const util::uuid& pUID) const noexcept;

	bool has_asset(const filesystem::path& pPath) const noexcept;
	bool has_asset(const util::uuid& pUID) const noexcept;
	bool has_asset(const asset::ptr& pAsset) const noexcept;

	bool remove_asset(const asset::ptr& pAsset);

	filesystem::path get_asset_path(const core::asset::ptr& pAsset) const;

	// Find and cast a resource asset.
	// Returns empty if it was not found.
	template <typename T = resource>
	resource_handle<T> get_resource(const filesystem::path& pPath) const
	{
		return{ get_asset(pPath) };
	}

	template <typename T = resource>
	resource_handle<T> get_resource(const util::uuid& pId) const
	{
		return{ get_asset(pId) };
	}

	void register_resource_factory(const std::string& pType, const resource_factory& pFactory);
	template <typename T>
	void register_default_resource_factory(const std::string& pType);

	template <typename Tcast = resource>
	Tcast* create_resource_for(const asset::ptr& pAsset) const
	{
		assert(pAsset);
		auto iter = mResource_factories.find(pAsset->get_type());
		if (iter != mResource_factories.end())
			iter->second(pAsset);
		return pAsset->get_resource<Tcast>();
	}

	// Set the root directory to find all assets.
	// Note: This affects the relative path of all assets.
	void set_root_directory(const filesystem::path& pPath);
	const filesystem::path& get_root_directory() const;

	// Load all assets in the root directory
	void load_assets();

	const asset_container& get_asset_list() const;

	// Create an asset representing a folder.
	asset::ptr create_folder(const filesystem::path& pPath);

	// Calls pCallable for each asset that is a child of pParent.
	template <typename Tcallable>
	void for_each_child(const asset::ptr& pParent, Tcallable&& pCallable) const;

	// Calls pCallable for each child and subchild of pParent.
	template <typename Tcallable>
	void for_each_child_recursive(const asset::ptr& pParent, Tcallable&& pCallable) const;

	bool has_children(const asset::ptr& pParent) const;
	bool has_subfolders(const asset::ptr& pParent) const;
	std::vector<asset::ptr> get_children(const asset::ptr& pParent) const;
	std::vector<asset::ptr> get_children_recursive(const asset::ptr& pParent) const;
	asset::ptr find_child(const asset::ptr& pParent, const std::string_view& pName) const;

	// Generates the name of the directory used to store an asset.
	// "dir.dir.name-000000000000"
	std::string generate_asset_directory_name(const asset::ptr& pAsset) const;

	filesystem::path create_asset_storage(const core::asset::ptr& pAsset) const;
	void store_asset(const core::asset::ptr& pAsset) const;
	bool rename_asset(const core::asset::ptr& pAsset, const std::string& pTo);
	bool move_asset(const core::asset::ptr& pAsset, const core::asset::ptr& pTo);
	void remove_asset_storage(const core::asset::ptr& pAsset) const;

	void update_directory_structure();
	void save_all_configuration();

	void remove_all_assets()
	{
		mAsset_list.clear();
	}

private:
	bool has_parent(const util::uuid& pTop, const util::uuid& pParent) const;
	// Turn an absolute path into a relative path to the root directory
	filesystem::path make_relative_to_root(const filesystem::path& pPath) const;

private:
	std::map<std::string, resource_factory> mResource_factories; // { [asset type], [factory] }
	std::vector<asset::ptr> mAsset_list;
	filesystem::path mRoot_dir;
	filesystem::filesystem_interface* mFilesystem{ nullptr };
};

template<typename T>
inline void asset_manager::register_default_resource_factory(const std::string& pType)
{
	register_resource_factory(pType, [](const asset::ptr& pAsset)
	{
		pAsset->set_resource(std::make_unique<T>());
	});
}

template<typename Tcallable>
inline void asset_manager::for_each_child(const asset::ptr& pParent, Tcallable&& pCallable) const
{
	if (pParent)
	{
		for (const auto& i : mAsset_list)
			if (i && i->get_parent_id() == pParent->get_id())
				pCallable(i);
	}
	else
	{
		for (const auto& i : mAsset_list)
			if (i && !i->get_parent_id().is_valid())
				pCallable(i);
	}
}

template<typename Tcallable>
inline void asset_manager::for_each_child_recursive(const asset::ptr& pParent, Tcallable&& pCallable) const
{
	for_each_child(pParent,
		[this, pCallable = std::forward<Tcallable>(pCallable)](const asset::ptr& pAsset)
	{
		pCallable(pAsset);
		for_each_child_recursive(pAsset, pCallable);
	});
}

} // namespace wge::core
