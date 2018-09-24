#pragma once

#include <wge/filesystem/path.hpp>

#include <string>
#include <memory>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace wge::core
{

typedef std::uint64_t asset_uid;

// Loads and saves the asset configuration
// file.
class asset_config
{
public:
	using on_save_metadata_callback = std::function<json()>();
	using on_load_metadata_callback = std::function<void(const json&)>();
	using ptr = std::shared_ptr<asset_config>;

	// Load the json for this asset
	void load(const json& pJson)
	{
		mType = pJson["type"];
		mID = pJson["id"];
		mDescription = pJson["description"];
		mMetadata = pJson["metadata"];
	}

	// Save any modified configuration for this asset
	json save() const
	{
		json result;
		result["type"] = mType;
		result["id"] = mID;
		result["metadata"] = mMetadata;
		return result;
	}

	void set_type(const std::string& pType)
	{
		mType = pType;
	}

	const std::string& get_type() const
	{
		return mType;
	}

	void set_id(asset_uid pID)
	{
		mID = pID;
	}

	asset_uid get_id() const
	{
		return mID;
	}

	// Set the absolute path to the configuration file
	void set_path(const filesystem::path& pPath)
	{
		mPath = pPath;
	}

	// Get the absolute path to the configuration file
	const filesystem::path& get_path() const
	{
		return mPath;
	}

	void set_metadata(const json& pJson)
	{
		mMetadata = pJson;
	}

	const json& get_metadata() const
	{
		return mMetadata;
	}

private:
	filesystem::path mPath;
	std::string mType;
	std::string mDescription;
	asset_uid mID;
	json mMetadata;

	friend class asset;
};

}