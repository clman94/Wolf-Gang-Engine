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
	using ptr = std::shared_ptr<asset_config>;

	// Load the json for this asset
	void load(const json& pJson);

	// Save any modified configuration for this asset
	json save() const;

	const std::string& get_type() const;
	void set_type(const std::string& pType);

	asset_uid get_id() const;
	void set_id(asset_uid pID);

	// Get the absolute path to the configuration file
	const filesystem::path& get_path() const;
	// Set the absolute path to the configuration file
	void set_path(const filesystem::path& pPath);

	const json& get_metadata() const;
	void set_metadata(const json& pJson);

private:
	filesystem::path mPath;
	std::string mType;
	std::string mDescription;
	asset_uid mID;
	json mMetadata;

	friend class asset;
};

}