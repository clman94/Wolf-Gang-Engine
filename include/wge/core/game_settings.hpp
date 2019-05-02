#pragma once

#include <wge/util/uuid.hpp>
#include <wge/util/json_helpers.hpp>
#include <wge/filesystem/file_input_stream.hpp>

#include <string>

namespace wge::core
{

class game_settings
{
public:
	bool load(const filesystem::path& pPath)
	{
		// pPath is pointing to a project.wge file directly.
		if (pPath.filename() == "project.wge")
		{
			mPath = pPath;

			// Set it to the root directory
			mAsset_directory = mPath;
			mAsset_directory.pop_filepath();
		}
		// pPath is pointing to a directory with a project.wge inside.
		else
		{
			mPath = pPath / "project.wge";
			mAsset_directory = pPath;
		}
		// The asset directory is the "assets" subdirectory.
		mAsset_directory /= "assets";

		// Make sure it exists, of course.
		if (!system_fs::exists(mPath))
			return false;

		// Read the file.
		std::ifstream stream(mPath.string().c_str());
		if (!stream)
			return false;

		json j = json::parse(stream);
		mName = j["name"];

		return true;
	}

	bool save()
	{
		json result;
		result["name"] = mName;

		filesystem::file_stream out;
		out.open(mPath, filesystem::stream_access::write);
		out.write(result.dump(2));
		return true;
	}

	bool save_new(const filesystem::path& pDirectory)
	{
		mPath = pDirectory / "project.wge";
		return save();
	}

	const std::string& get_name() const noexcept
	{
		return mName;
	}

	void set_name(const std::string& pName)
	{
		mName = pName;
	}

	const filesystem::path& get_asset_directory() const
	{
		return mAsset_directory;
	}

private:
	filesystem::path mPath;
	filesystem::path mAsset_directory;
	std::string mName;
	util::uuid mStart_scene;
};

} // namespace wge::core
