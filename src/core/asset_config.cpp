#include <wge/core/asset_config.hpp>

using namespace wge;
using namespace wge::core;

void asset_config::load(const json & pJson)
{
	mType = pJson["type"];
	mID = pJson["id"];
	mDescription = pJson["description"];
	mMetadata = pJson["metadata"];
}

json asset_config::save() const
{
	json result;
	result["type"] = mType;
	result["id"] = mID;
	result["description"] = mDescription;
	result["metadata"] = mMetadata;
	return result;
}

const std::string & asset_config::get_type() const
{
	return mType;
}

void asset_config::set_type(const std::string & pType)
{
	mType = pType;
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
