
#include <wge/filesystem/system_filesystem.hpp>
#include <wge/filesystem/filesystem_interface.hpp>
#include <wge/filesystem/file_input_stream.hpp>
#include <wge/filesystem/path.hpp>

using namespace wge;
using namespace wge::filesystem;

bool system_filesystem::exists(const path & pPath)
{
	return system_fs::exists(pPath.to_path());
}

path system_filesystem::get_working_directory()
{
	return system_fs::current_path();
}

path system_filesystem::get_absolute_path(const path & pPath)
{
	return system_fs::absolute(pPath.to_path());
}

input_stream::ptr system_filesystem::open(const path & pPath)
{
	std::shared_ptr<file_input_stream> stream = find_or_create_stream();
	if (stream->open(pPath))
	{
		mStreams.push_back(stream);
		return std::dynamic_pointer_cast<input_stream>(stream);
	}
	return nullptr;
}

std::shared_ptr<file_input_stream> system_filesystem::find_or_create_stream()
{
	for (auto& i : mStreams)
		if (i.use_count() == 1)
			return i;
	return std::make_shared<file_input_stream>();
}
