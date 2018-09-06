#pragma once

#include <memory>

#include <wge/filesystem/path.hpp>

namespace wge::filesystem
{

class input_stream;

class filesystem_interface
{
public:
	virtual ~filesystem_interface() {}

	// Check if pPath is a valid path
	virtual bool exists(const path& pPath) = 0;

	// Get the path to the current working directory
	virtual path get_working_directory() = 0;

	// Convert a path to an absolute path
	virtual path get_absolute_path(const path& pPath) = 0;

	// Open a new input stream
	virtual std::shared_ptr<input_stream> open(const path& pPath) = 0;
};

}
