#include <wge/core/asset_config.hpp>
#include <wge/util/hash.hpp>
#include <wge/filesystem/file_input_stream.hpp>

namespace wge::core
{

void asset_config::deserialize(const json & pJson)
{
	mType = pJson["type"];
	mID = pJson["id"];
	mDescription = pJson["description"];
	mMetadata = pJson["metadata"];
}

json asset_config::serialize() const
{
	json result;
	result["type"] = mType;
	result["id"] = mID;
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

void asset_config::generate_id()
{
	// Generate an id from a hash of the file path
	// TODO: Replace this with something that can generate a more "unique" id.
	mID = util::hash::hash64(mPath.string()) + reinterpret_cast<asset_uid>(this);
}

asset_uid asset_config::get_id() const
{
	return mID;
}

void asset_config::set_id(asset_uid pID)
{
	mID = pID;
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

} // namespace wge::core
