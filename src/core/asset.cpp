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

	try
	{
		// Load all the settings
		json j = json::parse(str);
		mName = j["name"];
		mType = j["type"];
		mId = j["id"];
		mDescription = j["description"];
		mMetadata = j["metadata"];
		mResource_metadata_cache = j["resource-metadata"];
		mParent = j["parent"];
		mLocation = primary_asset_location::create(pSystem_path.parent(), mName);

		update_resource_metadata();
	}
	catch (const json::exception& e)
	{
		log::error("In {}", pSystem_path.string());
		log::error("Error parsing asset configuration");
		log::error("{}", e.what());
		return false;
	}
	catch (...)
	{
		log::error("In {}", pSystem_path.string());
		log::error("Unknown error while parsing asset configuration");
	}
	return true;
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

void asset::save() const
{
	if (!mLocation && mResource)
	{
		mResource->save();
	}
	else
	{
		json j;
		j["name"] = mName;
		j["type"] = mType;
		j["id"] = mId;
		j["description"] = mDescription;
		j["metadata"] = mMetadata;
		if (mResource)
		{
			j["resource-metadata"] = mResource->serialize_data();
			mResource->save();
		}
		j["parent"] = mParent;

		filesystem::file_stream out;
		out.open(mLocation->get_autonamed_file(".wga"), filesystem::stream_access::write);
		out.write(j.dump(2));
	}
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
