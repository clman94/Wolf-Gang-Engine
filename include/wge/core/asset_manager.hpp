#pragma once

#include <wge/core/system.hpp>
#include <wge/core/asset.hpp>
#include <wge/filesystem/filesystem_interface.hpp>

#include <vector>
#include <map>
#include <iostream>

namespace wge::core
{

class asset_manager
{
public:
	using asset_factory = std::function<asset::ptr(const filesystem::path&, asset_config::ptr)>;
	using asset_container = std::vector<asset::ptr>;

	// TODO: Implement the filesystem_interface as the only means of
	//   loading assets.
	void set_filesystem(filesystem::filesystem_interface* pFilesystem);

	template <typename T>
	void register_asset(const std::string& pType);
	void register_asset(const std::string& pType, const asset_factory& pFactory);

	// Manually add an asset
	void add_asset(const asset::ptr& pAsset);

	// Find an asset by its relative path.
	// Returns empty if it is not found.
	asset::ptr find_asset(const filesystem::path& pPath) const noexcept;
	// Find an asset by its uid.
	// Returns empty when it it not found.
	asset::ptr find_asset(asset_uid pUID) const noexcept;

	bool has_asset(asset_uid pUID) const noexcept;
	bool has_asset(const asset::ptr& pAsset) const noexcept;

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

	// Register a new configuration asset
	void register_config_extension(const std::string& pType, const std::string& pExtension);
	// Register a new resource asset
	void register_resource_extension(const std::string& pType, const std::string& pExtension);

	// Set the root directory to find all assets.
	// Note: This affects the relative path of all assets.
	void set_root_directory(const filesystem::path& pPath);
	const filesystem::path& get_root_directory() const;

	// Load all assets in the root directory
	void load_assets();

	const asset_container& get_asset_list() const;

	// Create a configuration asset
	asset::ptr create_configuration_asset(const std::string& pType, const filesystem::path& pPath);

private:
	// Turn an absolute path into a relative path to the root directory
	filesystem::path make_relative_to_root(const filesystem::path& pPath) const;

	void load_configuration_asset(const filesystem::path& pPath, const std::string& pType);
	void load_resource_asset(const filesystem::path& pPath, const std::string& pType);

private:
	std::map<std::string, asset_factory> mAsset_factories;
	std::map<std::string, std::string> mAsset_resource_extensions; // { [extension], [asset type] }
	std::map<std::string, std::string> mAsset_config_extensions;
	std::vector<asset::ptr> mAsset_list;
	filesystem::path mRoot_dir;
	filesystem::filesystem_interface* mFilesystem{ nullptr };
};

template<typename T>
inline void asset_manager::register_asset(const std::string& pType)
{
	mAsset_factories[pType] =
		[](const filesystem::path& pPath, asset_config::ptr pConfig) -> asset::ptr
	{
		auto ptr = std::make_shared<T>(pConfig);
		ptr->set_path(pPath);
		return ptr;
	};
}

} // namespace wge::core
