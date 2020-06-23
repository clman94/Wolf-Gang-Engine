#pragma once

#include <wge/util/json_helpers.hpp>
#include <wge/filesystem/path.hpp>
#include <wge/util/uuid.hpp>
#include <wge/core/asset_location.hpp>
#include <wge/core/resource.hpp>

#include <memory>

namespace wge::core
{

// Assets are file-like objects that contain data and resources for a project.
class asset final
{
public:
	using ptr = std::shared_ptr<asset>;
	using wptr = std::weak_ptr<asset>;

	asset();

	// Load a file.
	bool load_file(const filesystem::path& pSystem_path);

	asset_location::ptr get_location() const noexcept
	{
		return mLocation;
	}

	// Save the assets configuration to its source file.
	void save() const;

	void save_to(const filesystem::path& pDirectory)
	{
		mLocation = primary_asset_location::create(pDirectory, mName);
		save();
	}

	const std::string& get_name() const noexcept;
	void set_name(const std::string& pName);

	const util::uuid& get_id() const noexcept;
	void set_id(const util::uuid& pId) noexcept
	{
		mId = pId;
	}

	const std::string& get_type() const noexcept;
	void set_type(const std::string& pType);

	const std::string& get_description() const noexcept;
	void set_description(const std::string& pDescription);

	const json& get_metadata() const noexcept;
	void set_metadata(const json& pJson);

	// Returns true if this asset stores resource data.
	bool is_resource() const noexcept;

	template <typename T = resource>
	T* get_resource() const noexcept
	{
		return dynamic_cast<T*>(mResource.get());
	}
	void set_resource(resource::uptr pResource) noexcept;
	
	const util::uuid& get_parent_id() const noexcept;
	void set_parent_id(const util::uuid& pId) noexcept;
	void set_parent(const asset::ptr& pAsset) noexcept;

	bool is_primary_asset() const noexcept
	{
		return std::dynamic_pointer_cast<primary_asset_location>(mLocation) != nullptr;
	}

	bool is_secondary_asset() const noexcept
	{
		return std::dynamic_pointer_cast<secondary_asset_location>(mLocation) != nullptr;
	}

	void set_save_configuration(bool pEnable_save) noexcept
	{
		mCan_save_configuration = pEnable_save;
	}

	bool is_save_configuration_enabled() const noexcept
	{
		return mCan_save_configuration;
	}
	
private:
	void update_resource_metadata() const;

private:
	bool mCan_save_configuration = true;

	util::uuid mParent;

	// Stores the resource.
	resource::uptr mResource;

	asset_location::ptr mLocation;

	// This is that path that the asset manager uses to
	// locate assets.
	std::string mName;

	// A string identifying the type of an asset.
	std::string mType;

	// An optional description of this asset.
	std::string mDescription;

	// The unique id that identifies this asset.
	util::uuid mId;

	// Any extra, structured, data that the user may want to store.
	// This is also used to store serialized data.
	// Note: Data specific to resources (such as a texture atlas),
	//   will be stored in the resource object itself.
	json mMetadata;

	// Resource-specific metadata is cached here until this asset is assigned a resource object.
	json mResource_metadata_cache;
};

// Keeps a reference to an asset but gives
// pointer-like access to a casted resource.
template <typename T>
class resource_handle
{
public:
	resource_handle() = default;
	resource_handle(const core::asset::ptr& pAsset) :
		mAsset(pAsset)
	{}

	T& operator*() const noexcept
	{
		assert(mAsset);
		return *mAsset->get_resource<T>();
	}

	T* operator->() const noexcept
	{
		assert(mAsset);
		return mAsset->get_resource<T>();
	}

	const asset::ptr& get_asset() const noexcept
	{
		return mAsset;
	}

	const util::uuid& get_id() const noexcept
	{
		assert(mAsset);
		return mAsset->get_id();
	}

	bool is_valid() const noexcept
	{
		return static_cast<bool>(mAsset);
	}

	void reset() noexcept
	{
		mAsset.reset();
	}

	operator bool() const noexcept
	{
		return is_valid();
	}

private:
	core::asset::ptr mAsset;
};

} // namespace wge::core
