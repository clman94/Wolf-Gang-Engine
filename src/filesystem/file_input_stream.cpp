
#include <wge/filesystem/file_input_stream.hpp>
#include <wge/filesystem/exception.hpp>

#include <sstream>

namespace wge::filesystem
{

file_stream::file_stream() :
	mAccess(stream_access::none),
	mLength(0)
{
	mStream.exceptions(std::fstream::failbit | std::fstream::badbit);
}

file_stream::~file_stream()
{
}

bool file_stream::open(const path& pPath, stream_access pAccess)
{
	auto flags = std::fstream::binary | std::fstream::ate;
	flags |= pAccess & stream_access::read ? std::fstream::in : 0;
	flags |= pAccess & stream_access::write ? std::fstream::out : 0;

	// Ensure the directory for the file exists if we are writing
	if (pAccess & stream_access::write)
	{
		filesystem::path directories = pPath;
		directories.pop_filepath();
		if (!directories.empty() && !std::filesystem::exists(directories))
			std::filesystem::create_directories(directories);
	}

	try {
		mStream.open(pPath.string().c_str(), flags);
	}
	catch (const std::fstream::failure& e)
	{
		throw io_error("Error opening stream for file \"" + pPath.string() + "\": (std::fstream::failure) " + std::string(e.what()));
	}

	if (mStream)
	{
		// Calculate the length of this file.
		mLength = tell();
		seek(0);
		mPath = pPath;
	}
	mAccess = pAccess;
	return mStream.good();
}

void file_stream::close()
{
	mStream.close();
}

std::size_t file_stream::seek(std::size_t pPos)
{
	try {
		if (mAccess & stream_access::read)
			mStream.seekg(pPos);
		if (mAccess & stream_access::write)
			mStream.seekp(pPos);
	}
	catch (const std::fstream::failure& e)
	{
		throw io_error("Error seeking to position " + std::to_string(pPos) + " in stream: (std::fstream::failure) " + std::string(e.what()));
	}
	return tell();
}

std::size_t file_stream::tell()
{
	try {
		if (mAccess & stream_access::read)
			return mStream.tellg();
		else
			return mStream.tellp();
	}
	catch (const std::fstream::failure& e)
	{
		throw io_error("Error telling to position of stream: (std::fstream::failure) " + std::string(e.what()));
	}
}

bool file_stream::is_eof()
{
	return mStream.eof();
}

bool file_stream::has_error()
{
	return mStream.fail() || mStream.bad();
}

bool file_stream::is_good()
{
	return !is_eof() || !has_error();
}

std::size_t file_stream::read(char* pData, std::size_t pRequested_size)
{
	if (!(mAccess & stream_access::read))
		throw io_error("Reading from stream with no read access");

	std::size_t last_position = tell();
	try
	{
		mStream.read(pData, pRequested_size);
	}
	catch (const std::fstream::failure& e)
	{
		throw io_error("Error reading from stream: (std::fstream::failure) " + std::string(e.what()));
	}
	return tell() - last_position;
}

std::string file_stream::read_all()
{
	try
	{
		std::stringstream sstr;
		sstr << mStream.rdbuf();
		return sstr.str();
	}
	catch (const std::fstream::failure& e)
	{
		throw io_error("Error reading from stream: (std::fstream::failure) " + std::string(e.what()));
	}
}

std::size_t file_stream::write(const char* pData, std::size_t pSize)
{
	if (!(mAccess & stream_access::write))
		throw io_error("Writing to stream with no write access");

	std::size_t last_position = tell();
	try
	{
		mStream.write(pData, pSize);
	}
	catch (const std::fstream::failure& e)
	{
		throw io_error("Error reading from stream: (std::fstream::failure) " + std::string(e.what()));
	}
	return tell() - last_position;
}

std::size_t file_stream::length()
{
	return mLength;
}

path file_stream::get_path()
{
	return mPath;
}

stream_access file_stream::get_access() const
{
	return mAccess;
}

} // namespace wge::filsystem
