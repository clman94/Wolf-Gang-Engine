#pragma once

#include <fstream>
#include <cstddef>

#include <wge/filesystem/input_stream.hpp>

namespace wge::filesystem
{

class file_input_stream :
	public input_stream
{
public:
	virtual ~file_input_stream();

	// Open a file as an input stream.
	// Returns true on success.
	bool open(const path& pPath);

	virtual void close() override;
	virtual std::size_t seek(std::size_t pPos) override;
	virtual std::size_t tell() override;
	virtual bool is_eof() override;
	virtual bool has_error() override;
	virtual bool is_good() override;
	virtual std::size_t read(unsigned char* pData, std::size_t pRequested_size) override;
	virtual std::size_t length() override;
	virtual path get_path() override;

private:
	path mPath;
	std::size_t mLength;
	std::ifstream mStream;
};

}