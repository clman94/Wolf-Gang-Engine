#include <wge/core/asset.hpp>
#include <wge/logging/log.hpp>
#include <wge/filesystem/file_input_stream.hpp>
#include <wge/filesystem/exception.hpp>

namespace wge::core
{

asset::asset() :
	mId(util::generate_uuid())
{}

void asset::serialize(json& pJson) const
{
	pJson["name"] = mName;
	pJson["type"] = mType;
	pJson["id"] = mId;
	pJson["description"] = mDescription;
	pJson["parent"] = mParent;
	pJson["data"] = nullptr;
	if (mResource)
	{
		pJson["data"] = mResource->serialize_data();
	}
}

void asset::deserialize(const json& pJson)
{
	mName = pJson["name"];
	mType = pJson["type"];
	mId = pJson["id"];
	mDescription = pJson["description"];
	mParent = pJson["parent"];
	if (mResource)
	{
		mResource->deserialize_data(util::json_alts(pJson, "data", "resource-metadata"));
	}
	else
	{
		mResource_metadata_cache = util::json_alts(pJson, "data", "resource-metadata");
	}
}

const std::string& asset::get_name() const noexcept
{
	return mName;
}

void asset::set_name(const std::string& pName)
{
	mName = pName;
}

const asset_id& asset::get_id() const noexcept
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

const std::string& asset::get_description() const noexcept
{
	return mDescription;
}

void asset::set_description(const std::string& pDescription)
{
	mDescription = pDescription;
}


bool asset::is_resource() const noexcept
{
	return (bool)mResource;
}

void asset::set_resource(resource::uptr pResource) noexcept
{
	mResource = std::move(pResource);
	update_resource_metadata();
}

const asset_id& asset::get_parent_id() const noexcept
{
	return mParent;
}

void asset::set_parent_id(const asset_id& pId) noexcept
{
	mParent = pId;
}

void asset::set_parent(const asset::ptr& pAsset) noexcept
{
	if (pAsset)
		mParent = pAsset->get_id();
	else
		mParent = util::uuid{};
}

void asset::update_resource_metadata() const
{
	if (mResource && !mResource_metadata_cache.is_null())
		mResource->deserialize_data(mResource_metadata_cache);
}

} // namespace wge::core
