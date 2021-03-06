#pragma once

#include <wge/util/json_helpers.hpp>
#include <wge/filesystem/path.hpp>
#include <wge/util/uuid.hpp>
#include <wge/core/asset_location.hpp>
#include <wge/core/resource.hpp>

#include <memory>

namespace wge::core
{

using asset_id = util::uuid;

// Assets are file-like objects that contain data and resources for a project.
class asset final
{
public:
	using ptr = std::shared_ptr<asset>;
	using wptr = std::weak_ptr<asset>;

	asset();

	asset_location::ptr get_location() const noexcept
	{
		return mLocation;
	}
	void set_location(const asset_location::ptr& pLocation) noexcept
	{
		mLocation = pLocation;
	}

	// Serializes the settings for this asset.
	// Use asset_manager::save_asset() to save assets.
	void serialize(json& pJson) const;
	void deserialize(const json& pJson);

	const std::string& get_name() const noexcept;
	void set_name(const std::string& pName);

	const asset_id& get_id() const noexcept;
	void set_id(const asset_id& pId) noexcept
	{
		mId = pId;
	}

	const std::string& get_type() const noexcept;
	void set_type(const std::string& pType);

	const std::string& get_description() const noexcept;
	void set_description(const std::string& pDescription);

	// Returns true if this asset stores resource data.
	bool is_resource() const noexcept;

	template <typename T = resource>
	T* get_resource() const noexcept
	{
		return dynamic_cast<T*>(mResource.get());
	}
	void set_resource(resource::uptr pResource) noexcept;
	
	const asset_id& get_parent_id() const noexcept;
	void set_parent_id(const asset_id& pId) noexcept;
	void set_parent(const asset::ptr& pAsset) noexcept;

	bool is_primary_asset() const noexcept
	{
		return std::dynamic_pointer_cast<primary_asset_location>(mLocation) != nullptr;
	}

	bool is_secondary_asset() const noexcept
	{
		return std::dynamic_pointer_cast<secondary_asset_location>(mLocation) != nullptr;
	}

private:
	void update_resource_metadata() const;

private:
	asset_id mParent;

	// Stores the resource.
	resource::uptr mResource;

	asset_location::ptr mLocation;

	// Name of the asset.
	std::string mName;

	// A string identifying the type of an asset.
	std::string mType;

	// An optional description of this asset.
	std::string mDescription;

	// The unique id that identifies this asset.
	asset_id mId;

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
	constexpr resource_handle(std::nullptr_t) noexcept {}

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

	const asset_id& get_id() const noexcept
	{
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
