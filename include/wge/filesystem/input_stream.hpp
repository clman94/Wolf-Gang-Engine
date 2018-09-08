#pragma once

#include <cstddef>
#include <memory>

#include <wge/filesystem/path.hpp>

namespace wge::filesystem
{

class input_stream
{
public:
	typedef std::shared_ptr<input_stream> ptr;

	virtual ~input_stream() {}

	// Close this stream
	virtual void close() = 0;

	// Seek to a position in stream. Returns
	// the actually position of the stream.
	virtual std::size_t seek(std::size_t pPos) = 0;

	// Returns the byte offset of the stream
	virtual std::size_t tell() = 0;

	// Returns true if this is at the end of
	// the stream and read(...) will return 0.
	virtual bool is_eof() = 0;
	// Returns true if this stream had an error.
	virtual bool has_error() = 0;
	// Returns true if all is good and you can read from it.
	virtual bool is_good() = 0;
	operator bool() { return is_good(); }

	// Returns the amount of bytes actually read.
	virtual std::size_t read(unsigned char* pData, std::size_t pRequested_size) = 0;

	// Get the length of this stream in bytes
	virtual std::size_t length() = 0;

	virtual path get_path() = 0;
};


}