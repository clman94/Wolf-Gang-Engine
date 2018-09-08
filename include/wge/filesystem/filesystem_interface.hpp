#pragma once

#include <memory>

#include <wge/filesystem/path.hpp>
#include <wge/filesystem/input_stream.hpp>

namespace wge::filesystem
{

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
	virtual input_stream::ptr open(const path& pPath) = 0;
};

}
