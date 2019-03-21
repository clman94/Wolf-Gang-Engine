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
	template<typename T>
	using tptr = std::shared_ptr<T>;
	using ptr = std::shared_ptr<resource>;

	virtual ~resource() {}

	virtual void load() {}
	virtual void unload() {}

	virtual bool is_loaded() const
	{
		return true;
	}

	// Returns metadata containing settings for this resource.
	virtual json get_metadata() const { return{}; }
	virtual void set_metadata(const json& pJson) {}
};

template <typename Tto, typename Tfrom>
[[nodiscard]] inline resource::tptr<Tto> cast_resource(const resource::tptr<Tfrom>& pFrom) noexcept
{
	return std::dynamic_pointer_cast<Tto>(pFrom);
}

// Assets are file-like objects that contain data and resources for a project.
class asset final
{
public:
	using ptr = std::shared_ptr<asset>;
	using wptr = std::weak_ptr<asset>;

	asset();

	// Load a file.
	bool load_file(const filesystem::path& pSystem_path);

	// Save the assets configuration to its file.
	void save() const;

	// This path will be used to locate this asset.
	const filesystem::path& get_path() const noexcept;
	void set_path(const filesystem::path& pPath);

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
	resource::tptr<T> get_resource() const noexcept
	{
		return cast_resource<T>(mResource);
	}
	void set_resource(const resource::ptr& pResource) noexcept;

private:
	void update_resource_metadata() const;

private:
	// Stores the resource.
	resource::ptr mResource;

	// This is that path that the asset manager uses to
	// locate assets.
	filesystem::path mPath;

	// This is the system path used to locate
	// the configuration file on the systems hard-drive.
	// May be empty if this asset does not come from the
	// systems hard-driive such as a pack file.
	filesystem::path mFile_path;

	// A string identifying the type of an asset.
	// TODO: Consider using something less heavy
	//   for quicker indexing.
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


} // namespace wge::core
