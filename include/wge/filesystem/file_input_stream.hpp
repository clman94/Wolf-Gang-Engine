#pragma once

#include <fstream>
#include <cstddef>

#include <wge/filesystem/input_stream.hpp>

namespace wge::filesystem
{

// Simplified wrapper around the std::fstream
class file_stream :
	public stream
{
public:
	file_stream();
	virtual ~file_stream();

	// Open a file as an input stream.
	// Returns true on success.
	bool open(const path& pPath, stream_access pAccess = stream_access::read_write);

	virtual void close() override;
	virtual std::size_t seek(std::size_t pPos) override;
	virtual std::size_t tell() override;
	virtual bool is_eof() override;
	virtual bool has_error() override;
	virtual bool is_good() override;

	virtual std::size_t read(char* pData, std::size_t pRequested_size) override;
	std::string read_all();
	virtual std::size_t write(const char* pData, std::size_t pSize) override;
	using stream::write;

	virtual std::size_t length() override;
	virtual path get_path() override;
	virtual stream_access get_access() const override;

private:
	stream_access mAccess;
	path mPath;
	std::size_t mLength;
	std::fstream mStream;
};

} // namespace wge::filesystem
