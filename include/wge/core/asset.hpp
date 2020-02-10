#pragma once

#include <wge/util/json_helpers.hpp>
#include <wge/filesystem/path.hpp>
#include <wge/util/uuid.hpp>

#include <memory>

namespace wge::core
{

class asset_location
{
public:
	using ptr = std::shared_ptr<asset_location>;

	static ptr create(const filesystem::path& pDirectory, const std::string& pName)
	{
		return std::make_shared<asset_location>(pDirectory, pName);
	}

	asset_location(const filesystem::path& pDirectory, const std::string& pName) :
		mDirectory(pDirectory),
		mName(pName)
	{}
	asset_location(asset_location&&) = default;
	asset_location(const asset_location&) = delete;

	filesystem::path get_autonamed_file(const std::string& pExtension) const
	{
		assert(is_valid());
		assert(!pExtension.empty());
		return mDirectory / (mName + pExtension);
	}

	filesystem::path get_file(const std::string& pFilename) const
	{
		assert(is_valid());
		assert(!pFilename.empty());
		return mDirectory / pFilename;
	}

	bool remove_autonamed_file(const std::string& pExtension)
	{
		assert(is_valid());
		assert(!pExtension.empty());
		system_fs::remove(get_autonamed_file(pExtension));
	}

	bool remove_file(const std::string& pFilename)
	{
		assert(is_valid());
		assert(!pFilename.empty());
		system_fs::remove(get_file(pFilename));
	}

	// Moves the directory and renames the autonamed files.
	void move_to(const filesystem::path& pDirectory, const std::string& pName)
	{
		assert(is_valid());
		assert(!pDirectory.empty());
		assert(!pName.empty());

		// Move the directory.
		system_fs::rename(mDirectory, pDirectory);
		mDirectory = pDirectory;

		// Get all autonamed files with the old name.
		std::vector<filesystem::path> autonamed_files;
		for (auto i : system_fs::directory_iterator(pDirectory))
		{
			if (i.path().stem() == mName)
				autonamed_files.push_back(i.path());
		}

		// Rename those autonamed files with the new name.
		mName = pName;
		for (auto& i : autonamed_files)
			system_fs::rename(i, get_autonamed_file(i.extension()));
	}

	const std::string& get_name() const noexcept
	{
		assert(is_valid());
		return mName;
	}

	const filesystem::path& get_directory() const noexcept
	{
		assert(is_valid());
		return mDirectory;
	}

	bool is_valid() const noexcept
	{
		return !mName.empty() && !mDirectory.empty();
	}

private:
	std::string mName;
	filesystem::path mDirectory;
};

class resource
{
public:
	using uptr = std::unique_ptr<resource>;

	virtual ~resource() {}

	void set_location(const asset_location::ptr& pLocation)
	{
		mLocation = pLocation;
	}

	bool has_location() const noexcept
	{
		return mLocation != nullptr;
	}

	asset_location& get_location() noexcept
	{
		assert(has_location());
		return *mLocation;
	}

	const asset_location& get_location() const noexcept
	{
		assert(has_location());
		return *mLocation;
	}

	void load(const asset_location::ptr& pLocation)
	{
		set_location(pLocation);
		load();
	}

	virtual void load() {}
	virtual void unload() {}

	virtual bool is_loaded() const
	{
		return true;
	}

	virtual void save() {}

	// Serialize any settings from this resource.
	virtual json serialize_data() const { return{}; }
	virtual void deserialize_data(const json& pJson) {}

private:
	asset_location::ptr mLocation;
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

	asset_location::ptr get_location() const noexcept
	{
		return mLocation;
	}

	// Save the assets configuration to its source file.
	void save() const;

	void save_to(const filesystem::path& pDirectory)
	{
		mLocation = asset_location::create(pDirectory, mName);
		save();
	}

	const std::string& get_name() const noexcept;
	void set_name(const std::string& pName);

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
