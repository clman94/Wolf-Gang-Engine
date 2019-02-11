#pragma once

#include <wge/filesystem/path.hpp>
#include <wge/util/uuid.hpp>

#include <string>
#include <memory>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace wge::core
{

// Loads and saves the asset configuration
// file.
//
// The format is as follows:
//
// type :
//   A string that represents the type of the asset.
//   This will dictate the loader that will
//   be called to load the asset.
//
// id :
//   A unique id for universal access to an asset.
//   This id will never change for an asset after it
//   is generated so you can access the same asset even
//   if its path changes.
//
// description :
//   A user defined piece of text to help document this asset.
//
// metadata :
//   This is the configuration the asset stores. E.g. texture atlas,
//   serialized data, etc...
//
class asset_config final
{
public:
	using ptr = std::shared_ptr<asset_config>;

	// Load the json for this asset
	void deserialize(const json& pJson);

	// Save any modified configuration for this asset
	json serialize() const noexcept;

	// Save the serialized data to a file specified by the path.
	void save() const;

	const std::string& get_type() const;
	void set_type(const std::string& pType);

	const util::uuid& get_id() const noexcept;
	void set_id(const util::uuid& pId) noexcept;

	// Get the absolute path to the configuration file
	const filesystem::path& get_path() const;
	// Set the absolute path to the configuration file
	void set_path(const filesystem::path& pPath);

	const json& get_metadata() const;
	void set_metadata(const json& pJson);

	const std::string& get_description() const;
	void set_description(const std::string& pDescription);

private:
	filesystem::path mPath;
	std::string mType;
	std::string mDescription;
	util::uuid mId;
	json mMetadata;

	friend class asset;
};

} // namespace wge::core
