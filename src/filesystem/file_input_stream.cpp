
#include <wge/filesystem/file_input_stream.hpp>

using namespace wge;
using namespace wge::filesystem;

file_input_stream::~file_input_stream()
{
	close();
}

bool file_input_stream::open(const path & pPath)
{
	auto flags = std::fstream::binary | std::fstream::in | std::fstream::ate;
	mStream.open(pPath.string().c_str(), flags);
	if (mStream)
	{
		mLength = mStream.tellg();
		mStream.seekg(0);
		mPath = pPath;
	}
	return mStream.good();
}

void file_input_stream::close()
{
	mStream.close();
}

std::size_t file_input_stream::seek(std::size_t pPos)
{
	mStream.seekg(pPos);
	return tell();
}

std::size_t file_input_stream::tell()
{
	return mStream.tellg();
}

bool file_input_stream::is_eof()
{
	return mStream.eof();
}

bool file_input_stream::has_error()
{
	return mStream.fail() || mStream.bad();
}

bool file_input_stream::is_good()
{
	return !is_eof() || !has_error();
}

std::size_t file_input_stream::read(unsigned char * pData, std::size_t pRequested_size)
{
	std::size_t last_position = tell();
	mStream.read(reinterpret_cast<char*>(pData), pRequested_size);
	return tell() - last_position;
}

std::size_t file_input_stream::length()
{
	return mLength;
}

path file_input_stream::get_path()
{
	return mPath;
}
