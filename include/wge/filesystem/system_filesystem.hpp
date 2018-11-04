#pragma once

#include <memory>

#include <wge/filesystem/filesystem_interface.hpp>
#include <wge/filesystem/input_stream.hpp>
#include <wge/filesystem/file_input_stream.hpp>
#include <wge/filesystem/path.hpp>

namespace wge::filesystem
{

class system_filesystem :
	public filesystem_interface
{
public:
	virtual ~system_filesystem() {}

	// Check if pPath is a valid path
	virtual bool exists(const path& pPath) override;

	// Get the path to the current working directory
	virtual path get_working_directory() override;

	// Convert a path to an absolute path
	virtual path get_absolute_path(const path& pPath) override;

	// Open a new input stream
	virtual stream::ptr open(const path& pPath) override;

private:
	std::shared_ptr<file_stream> find_or_create_stream();

private:
	std::vector<std::shared_ptr<file_stream>> mStreams;
};

} // namespace wge::filesystem
