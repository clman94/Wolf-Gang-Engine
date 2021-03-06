#pragma once

#include <cstddef>
#include <memory>

#include <wge/filesystem/path.hpp>
#include <wge/util/enum.hpp>

namespace wge::filesystem
{

enum class stream_access : util::flag_type
{
	none = 0,
	read = 1,
	write = 2,
	read_write = 3,
};
ENUM_CLASS_FLAG_OPERATORS(stream_access);

// Base class for byte stream objects.
// Throws io_error on failures.
class stream
{
public:
	using ptr = std::shared_ptr<stream>;

	virtual ~stream() {}

	// Close this stream
	virtual void close() = 0;

	virtual stream_access get_access() const = 0;

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
	// Returns true if all is good and you can read/write from it.
	virtual bool is_good() = 0;
	operator bool() { return is_good(); }

	// Get the length of this stream in bytes
	virtual std::size_t length() = 0;

	// Get path of stream
	virtual path get_path() = 0;

	// Returns the amount of bytes actually read.
	virtual std::size_t read(char* pData, std::size_t pRequested_size) = 0;
	// Returns the amount of bytes actually written.
	virtual std::size_t write(const char* pData, std::size_t pSize) = 0;
	std::size_t write(const std::string& pData)
	{
		return write(pData.c_str(), pData.size());
	}
};

} // namespace wge::filesystem
