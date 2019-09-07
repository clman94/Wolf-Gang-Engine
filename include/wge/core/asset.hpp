#pragma once

#include <wge/util/json_helpers.hpp>
#include <wge/filesystem/path.hpp>
#include <wge/util/uuid.hpp>

#include <memory>

namespace wge::core
{

class resource
{
public:
	using uptr = std::unique_ptr<resource>;

	virtual ~resource() {}

	virtual void load(const filesystem::path& pDirectory, const std::string& pName) {}
	virtual void unload() {}

	virtual bool is_loaded() const
	{
		return true;
	}

	virtual void save() {}

	// Make sure the resource is unloaded when you move it around
	// so any streams are cleaned up. This function should be used
	// to notify this resource about the change in path.
	virtual void update_source_path(const filesystem::path& pDirectory, const std::string& mName) {}

	// Serialize any settings from this resource.
	virtual json serialize_data() const { return{}; }
	virtual void deserialize_data(const json& pJson) {}
};

// Assets are file-like objects that contain data and resources for a project.
class asset final
{
public:
	using ptr = std::shared_ptr<asset>;
	using wptr = std::weak_ptr<asset>;

	asset();

	// Load a file.
	bool load_file(const filesystem::path& pSystem_path);

	// Save the assets configuration to its source file.
	void save() const;

	const std::string& get_name() const noexcept;
	void set_name(const std::string& pName);

	// Get path to the configuration file on the hard-drive.
	const filesystem::path& get_file_path() const noexcept;
	void set_file_path(const filesystem::path& pPath);

	const util::uuid& get_id() const noexcept;

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

private:
	void update_resource_metadata() const;

private:
	util::uuid mParent;

	// Stores the resource.
	resource::uptr mResource;

	// This is that path that the asset manager uses to
	// locate assets.
	std::string mName;

	// File path to the asset's source file on the systems disk.
	filesystem::path mFile_path;

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

	bool is_valid() const noexcept
	{
		return static_cast<bool>(mAsset);
	}

	operator bool() const noexcept
	{
		return is_valid();
	}

private:
	core::asset::ptr mAsset;
};

} // namespace wge::core
