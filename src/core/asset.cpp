#include <wge/core/asset.hpp>
#include <wge/logging/log.hpp>
#include <wge/filesystem/file_input_stream.hpp>
#include <wge/filesystem/exception.hpp>

namespace wge::core
{

asset::asset() :
	mId(util::generate_uuid())
{}

bool asset::load_file(const filesystem::path& pSystem_path)
{
	// Read the file
	std::ifstream stream(pSystem_path.string().c_str());
	if (!stream)
		return false;
	std::string str(std::istreambuf_iterator<char>(stream), {});

	mFile_path = pSystem_path;

	// Load all the settings
	json data = json::parse(str);
	mType = data["type"];
	mId = data["id"];
	mDescription = data["description"];
	mMetadata = data["metadata"];
	mResource_metadata_cache = data["resource-metadata"];
	update_resource_metadata();
	return true;
}

const filesystem::path& asset::get_path() const noexcept
{
	return mPath;
}

void asset::set_path(const filesystem::path& pPath)
{
	mPath = pPath;
}

const filesystem::path& asset::get_file_path() const noexcept
{
	return mFile_path;
}

void asset::set_file_path(const filesystem::path & pPath)
{
	mFile_path = pPath;
}

const util::uuid& asset::get_id() const noexcept
{
	return mId;
}

const std::string& asset::get_type() const noexcept
{
	return mType;
}

void asset::set_type(const std::string& pType)
{
	mType = pType;
}

void asset::save() const
{
	assert(!mFile_path.empty());

	json data;
	data["type"] = mType;
	data["id"] = mId;
	data["description"] = mDescription;
	data["metadata"] = mMetadata;
	if (mResource)
	{
		data["resource-metadata"] = mResource->get_metadata();
		mResource->save();
	}

	filesystem::file_stream out;
	out.open(mFile_path, filesystem::stream_access::write);
	out.write(data.dump(2));
}

const std::string& asset::get_description() const noexcept
{
	return mDescription;
}

void asset::set_description(const std::string& pDescription)
{
	mDescription = pDescription;
}

const json& asset::get_metadata() const noexcept
{
	return mMetadata;
}

void asset::set_metadata(const json& pJson)
{
	mMetadata = pJson;
}

bool asset::is_resource() const noexcept
{
	return (bool)mResource;
}

void asset::set_resource(const resource::ptr & pResource) noexcept
{
	mResource = pResource;
	update_resource_metadata();
}

void asset::update_resource_metadata() const
{
	if (mResource)
		mResource->set_metadata(mResource_metadata_cache);
}

} // namespace wge::core
