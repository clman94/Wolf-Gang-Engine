#include <wge/core/asset_config.hpp>
#include <wge/util/hash.hpp>
#include <wge/filesystem/file_input_stream.hpp>

namespace wge::core
{

void asset_config::deserialize(const json & pJson)
{
	mType = pJson["type"];
	mId = pJson["id"];
	mDescription = pJson["description"];
	mMetadata = pJson["metadata"];
}

json asset_config::serialize() const noexcept
{
	json result;
	result["type"] = mType;
	result["id"] = mId;
	result["description"] = mDescription;
	result["metadata"] = mMetadata;
	return result;
}

void asset_config::save() const
{
	filesystem::file_stream out;
	out.open(mPath, filesystem::stream_access::write);
	out.write(serialize().dump(2));
}

const std::string & asset_config::get_type() const
{
	return mType;
}

void asset_config::set_type(const std::string & pType)
{
	mType = pType;
}

const util::uuid& asset_config::get_id() const noexcept
{
	return mId;
}

void asset_config::set_id(const util::uuid& pId) noexcept
{
	mId = pId;
}

const filesystem::path & asset_config::get_path() const
{
	return mPath;
}

void asset_config::set_path(const filesystem::path & pPath)
{
	mPath = pPath;
}

const json & asset_config::get_metadata() const
{
	return mMetadata;
}


void asset_config::set_metadata(const json & pJson)
{
	mMetadata = pJson;
}

const std::string& asset_config::get_description() const
{
	return mDescription;
}

void asset_config::set_description(const std::string& pDescription)
{
	mDescription = pDescription;
}

} // namespace wge::core
