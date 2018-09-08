#pragma once

#include <memory>

#include <wge/filesystem/filesystem_interface.hpp>

namespace wge::filesystem
{

class path;
class input_stream;
class file_input_stream;

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
	virtual input_stream::ptr open(const path& pPath) override;

private:
	std::shared_ptr<file_input_stream> find_or_create_stream();

private:
	std::vector<std::shared_ptr<file_input_stream>> mStreams;
};

}