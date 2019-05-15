#pragma once

#include <wge/core/asset.hpp>
#include <wge/core/serialize_type.hpp>
#include <wge/core/system.hpp>
#include <wge/filesystem/filesystem_interface.hpp>
#include <wge/util/ptr.hpp>
#include <wge/filesystem/file_structure.hpp>

#include <vector>
#include <map>
#include <iostream>

namespace wge::core
{

class asset_manager
{
public:
	using resource_factory = std::function<void(asset::ptr&)>;
	using asset_container = std::vector<asset::ptr>;
	using file_structure = filesystem::file_structure<asset::ptr>;

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

	// Find and cast a resource asset.
	// Returns empty if it was not found.
	template <typename T = resource>
	resource::tptr<T> get_resource(const filesystem::path& pPath) const;

	void register_resource_factory(const std::string& pType, const resource_factory& pFactory);

	// Set the root directory to find all assets.
	// Note: This affects the relative path of all assets.
	void set_root_directory(const filesystem::path& pPath);
	const filesystem::path& get_root_directory() const;

	// Load all assets in the root directory
	void load_assets();

	const asset_container& get_asset_list() const;

	asset::ptr import_resource(const filesystem::path& pResource_path, const std::string& pType);
	void import_all_with_ext(const std::string& pExtension, const std::string& pType);

	asset::ptr create_asset(const filesystem::path& pPath, const std::string& pType, const json& pMetadata = {});

	const file_structure& get_file_structure() const
	{
		return mFile_structure;
	}

private:
	// Turn an absolute path into a relative path to the root directory
	filesystem::path make_relative_to_root(const filesystem::path& pPath) const;

private:
	std::map<std::string, resource_factory> mResource_factories; // { [asset type], [factory] }
	std::vector<asset::ptr> mAsset_list;
	file_structure mFile_structure;
	filesystem::path mRoot_dir;
	filesystem::filesystem_interface* mFilesystem{ nullptr };
};

template<typename T>
inline resource::tptr<T> asset_manager::get_resource(const filesystem::path& pPath) const
{
	asset::ptr ptr = get_asset(pPath);
	if (!ptr)
		return{};
	return ptr->get_resource<T>();
}

} // namespace wge::core
