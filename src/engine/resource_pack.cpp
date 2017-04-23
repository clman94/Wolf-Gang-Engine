#include <string>
#include <vector>
#include <istream>
#include <cstdint>
#include <fstream>
#include <engine/filesystem.hpp>
#include <engine/utility.hpp>
#include <engine/resource_pack.hpp>


// Read an unsigned integer value (of any size) from a stream.
// Format is little endian (for convenience)
template<typename T>
inline T read_unsignedint_binary(std::istream& pStream)
{
	char bytes[sizeof(T)];
	if (!pStream.read(bytes, sizeof(T)))
		return 0;

	T val = 0;
	for (size_t i = 0; i < sizeof(T); i++)
		val += (T)bytes[i] << (8 * i);
	return val;
}

// Stores an unsigned integer value (of any size) to a stream.
// Format is little endian (for convenience)
template<typename T>
inline bool write_unsignedint_binary(std::ostream& pStream, const T pVal)
{
	char bytes[sizeof(T)];
	for (size_t i = 0; i < sizeof(T); i++)
		bytes[i] = (pVal & (0xFF << (8 * i))) >> (8 * i);
	pStream.write(bytes, sizeof(T));
	return pStream.good();
}

// Test
/*int main()
{
    
    std::stringstream stream(std::ios_base::binary|std::ios_base::in|std::ios_base::out);
    write_unsignedint_binary<uint32_t>(stream, 10000000);
    std::cout << stream.str().size() << "\n" << "\n";
    for (unsigned char i : stream.str())
    {
        std::cout << (unsigned int)i << "\n";
    }
    
    stream.seekg(0);
    uint32_t read = read_unsignedint_binary<uint32_t>(stream);
    std::cout << "\n" << read;
}*/

class packing_ignore
{
public:

	/*
	Pack Settings Format

	[whitelist or blacklist]
	file <path>
	 ...
	dir <path>
	 ...
	*/

	packing_ignore()
	{
		mIs_valid = false;
	}

	bool open(const encoded_path& pPath)
	{
		mIs_valid = false;
		std::ifstream stream(pPath.string().c_str());
		if (!stream)
			return false;
		std::string list_type;
		std::getline(stream, list_type);
		if (list_type == "whitelist")
			mIs_whitelist = true;
		else if (list_type == "blacklist")
			mIs_whitelist = false;
		else
		{
			util::error("Please specify 'blacklist' or 'whitelist'");
			return false;
		}

		const encoded_path parent_folder(pPath.parent());
		for (std::string line; std::getline(stream, line);)
		{
			const encoded_path path(parent_folder / line);
			const std::string path_string = path.string();
			if (!engine::fs::exists(path_string))
			{
				util::warning("File does not exist '" + path_string + "'");
				continue;
			}

			entry nentry;
			nentry.is_directory = engine::fs::is_directory(path.string());
			nentry.path = path;
			mFiles.push_back(std::move(nentry));
		}
		mIs_valid = true;
		return true;
	}

	bool is_file_valid(const encoded_path& pPath) const
	{
		if (!is_valid())
			return false;

		bool has_entry = false;
		for (const auto& i : mFiles)
		{
			if ((!i.is_directory && i.path == pPath)
				|| (i.is_directory && pPath.has_directory(i.path)))
			{
				has_entry = true;
				break;
			}
		}
		return has_entry == mIs_whitelist;
	}

	bool is_valid() const
	{
		return mIs_valid;
	}

private:

	struct entry
	{
		bool is_directory;
		encoded_path path;
	};

	bool mIs_whitelist;
	bool mIs_valid;
	std::vector<entry> mFiles;
};

inline bool append_stream(std::ostream& pDest, std::istream& pSrc)
{
	auto end = pSrc.tellg();
	pSrc.seekg(0);
	while (pSrc.good() && pDest)
	{
		const int max_read = 1024;
		char data[max_read];
		if (end - pSrc.tellg() >= max_read) // A 1024 byte chunk
		{
			pSrc.read(data, max_read);
			pDest.write(data, max_read);
		}
		else { // Remainder
			const uint64_t remainder = end - pSrc.tellg();
			pSrc.read(data, remainder);
			pDest.write(data, remainder);
			return true;
		}
		pDest.flush();
	}

	return true;
}

bool create_resource_pack(const std::string& pSrc_directory, const std::string& pDest)
{
	const encoded_path root_dir(engine::fs::absolute(pSrc_directory).string());

	packing_ignore ignore_list;

	const std::string ignorelist_path = engine::fs::absolute("./data/pack_ignore.txt").string();
	if (ignore_list.open(ignorelist_path))
	{
		util::info("Loaded ignore list '" + ignorelist_path + "'");
	}
	else
	{
		util::warning("Couldn't load ignore list '" + ignorelist_path + "'");
		util::info("If you didn't specify a list, you can ignore the previous warning.");
	}

	// Get list of files to add
	std::vector<engine::fs::path> files_to_add;
	for (auto i : engine::fs::recursive_directory_iterator(pSrc_directory))
	{
		auto absolute = engine::fs::absolute(i.path());
		if (!engine::fs::is_directory(absolute)
			&& (!ignore_list.is_valid() || ignore_list.is_file_valid(absolute.string())))
			files_to_add.push_back(absolute);
	}

	// Create header
	pack_header header;
	uint64_t current_position = 0;
	for (auto i : files_to_add)
	{
		encoded_path path(i.string());
		path.snip_path(root_dir);

		pack_header::file_info file;
		file.path = path;
		file.size = engine::fs::file_size(i);
		file.position = current_position;
		current_position += file.size;
		header.add_file(file);
	}

	// Start stream for the destination
	std::ofstream stream(pDest.c_str()
		, std::fstream::binary);
	if (!stream)
		return false;

	// Add header
	header.generate(stream);
	stream.flush();

	// Add file data
	for (auto i : files_to_add)
	{
		std::ifstream file_stream(i.string().c_str(), std::fstream::binary | std::fstream::ate);
		if (!file_stream)
			continue;
		util::info("Packing file '" + i.string() + "'");
		append_stream(stream, file_stream);
	}

	return true;
}

class resource_pack
{
public:

};

encoded_path::encoded_path(const std::string & pString)
{
	parse(pString);
}

bool encoded_path::parse(const std::string & pString)
{
	if (pString.empty())
		return false;
	mParent_directories.clear();
	auto start_segment = pString.begin();
	auto end_segment = pString.begin();
	for (; end_segment != pString.end(); end_segment++)
	{
		if (*end_segment == '\\' ||
			*end_segment == '/')
		{
			if (start_segment < end_segment - 1) // Segment is not empty
			{
				std::string segment(start_segment, end_segment);
				if (segment != ".") // Not a redundant current directory thing
					mParent_directories.push_back(segment);
				else if (segment == ".." && !mParent_directories.empty()) // Go back if possible
				{
					if (mParent_directories.empty())
						mParent_directories.pop_back();
				}
			}
			start_segment = end_segment + 1;
		}
	}
	if (start_segment < end_segment - 1) // Last segment is not empty
	{
		mParent_directories.push_back(std::string(start_segment, end_segment)); // Get last segment
	}
	if (!mParent_directories.empty())
	{
		mFilename = mParent_directories.back();
		mParent_directories.erase(mParent_directories.end() - 1);
	}
	return true;
}

bool encoded_path::in_directory(const encoded_path & pPath) const
{
	if (mParent_directories.size() != pPath.mParent_directories.size() + 1)
		return false;
	if (mParent_directories.back() != pPath.mFilename)
		return false;

	for (size_t i = 0; i < mParent_directories.size(); i++)
	{
		if (mParent_directories[i] != pPath.mParent_directories[i])
			return false;
	}

	return true;
}

bool encoded_path::has_directory(const encoded_path & pPath) const
{
	if (pPath.mParent_directories.size() + 1 > mParent_directories.size())
		return false;

	size_t i = 0;
	for (; i < pPath.mParent_directories.size(); i++)
	{
		if (mParent_directories[i] != pPath.mParent_directories[i])
			return false;
	}
	if (mParent_directories[i] != pPath.mFilename)
		return false;
	return true;
}

bool encoded_path::snip_path(const encoded_path & pPath)
{
	if (!has_directory(pPath))
		return false;
	mParent_directories.erase(mParent_directories.begin()
		, mParent_directories.begin() + pPath.mParent_directories.size() + 1);
	return true;
}

std::string encoded_path::string() const
{
	std::string retval;
	for (auto i : mParent_directories)
	{
		retval += i + "/"; // Supported by windows and linux, however may still not be entirely portable.
						   // TODO: Add preprocessor directives to check for type of system
	}
	retval += mFilename;
	return retval;
}

bool encoded_path::is_same(const encoded_path & pPath) const
{
	if (pPath.mParent_directories.size() != mParent_directories.size())
		return false;
	if (pPath.mFilename != mFilename)
		return false;
	for (size_t i = 0; i < mParent_directories.size(); i++)
		if (pPath.mParent_directories[i] != mParent_directories[i])
			return false;
	return true;
}

void encoded_path::append(const encoded_path & pRight)
{
	mParent_directories.push_back(mFilename);
	mFilename = pRight.mFilename;
	mParent_directories.insert(mParent_directories.end(), pRight.mParent_directories.begin(), pRight.mParent_directories.end());
}

bool encoded_path::pop_filename()
{
	if (mParent_directories.size() == 0)
		return false;
	mFilename = mParent_directories.back();
	mParent_directories.pop_back();
	return true;
}

void pack_header::add_file(file_info pFile)
{
	mFiles.push_back(pFile);
}

bool pack_header::generate(std::ostream & pStream) const
{
	auto start = pStream.tellp();
	write_unsignedint_binary<uint64_t>(pStream, 0); // Placeholder
	write_unsignedint_binary<uint64_t>(pStream, mFiles.size());
	for (auto& i : mFiles)
	{
		std::string path = i.path.string();
		write_unsignedint_binary<uint16_t>(pStream, static_cast<uint16_t>(path.size()));
		pStream.write(path.c_str(), path.size());

		write_unsignedint_binary<uint64_t>(pStream, i.position);
		write_unsignedint_binary<uint64_t>(pStream, i.size);
	}
	auto end = pStream.tellp();
	pStream.seekp(start);
	write_unsignedint_binary<uint64_t>(pStream, end - start - 1);
	pStream.seekp(end);
	return true;
}

bool pack_header::parse(std::istream & pStream)
{
	pStream.seekg(sizeof(uint64_t)); // Skip the first 8 bytes
	uint64_t file_count = read_unsignedint_binary<uint64_t>(pStream);
	if (file_count == 0)
		return false;

	for (uint64_t i = 0; i < file_count; i++)
	{
		file_info file;

		// Get path
		uint16_t path_size = read_unsignedint_binary<uint16_t>(pStream);
		std::string path;
		path.resize(path_size);
		if (!pStream.read(&path[0], path_size))
			return false;
		file.path = path;

		file.position = read_unsignedint_binary<uint64_t>(pStream);
		file.size = read_unsignedint_binary<uint64_t>(pStream);
	}
	mHeader_size = pStream.tellg().seekpos();
	return true;
}

util::optional<pack_header::file_info> pack_header::get_file(const encoded_path & pPath) const
{
	for (auto& i : mFiles)
	{
		if (i.path == pPath)
			return i;
	}
	return{};
}

uint64_t pack_header::get_header_size() const
{
	return mHeader_size;
}

std::vector<char> pack_stream::read(uint64_t pCount)
{
	if (!is_valid())
		return{};

	// Check bounds
	if (mStream.tellg().seekpos() + pCount - mHeader_offset
		>= mFile.position + mFile.size)
		return{};
	std::vector<char> retval;
	retval.resize(static_cast<size_t>(pCount));
	mStream.read(retval.data(), static_cast<size_t>(pCount));
	return retval;
}

bool pack_stream::read(char * pData, uint64_t pCount)
{
	if (!is_valid())
		return false;

	// Check bounds
	if (tell() + pCount >= mFile.size)
		return false;
	mStream.read(pData, pCount);
	return true;
}

bool pack_stream::seek(uint64_t pPosition)
{
	if (!is_valid())
		return false;

	if (pPosition <= mFile.size)
		return false;
	mStream.seekg(mFile.position + pPosition + mHeader_offset);
	return true;
}

uint64_t pack_stream::tell()
{
	if (!is_valid())
		return 0;

	return mStream.tellg().seekpos() - mFile.position - mHeader_offset;
}

bool pack_stream::is_valid()
{
	return mStream.good() && tell() < mFile.position + mFile.size;
}

bool pack_stream::open(const std::string & pPack, const std::string & pFile)
{
	std::ifstream stream(pPack, std::fstream::binary);
	if (!stream)
		return false;

	pack_header header;
	if (!header.parse(stream))
		return false;

	auto file = header.get_file(pFile);
	if (!file)
		return false;

	mStream.open(file->path.string().c_str(), std::fstream::binary);
	if (!mStream)
		return false;
	mStream.seekg(file->position);
	mHeader_offset = header.get_header_size();
	return true;
}

bool pack_stream_factory::open(const std::string & pPath)
{
	std::ifstream stream(pPath, std::fstream::binary);
	if (!stream)
		return false;
	mPath = pPath;
	return mHeader.parse(stream);
}

pack_stream pack_stream_factory::open_file(const std::string & pPath)
{
	auto file = mHeader.get_file(pPath);
	if (!file)
		return{};
	pack_stream stream;
	stream.mStream.open(mPath.c_str(), std::fstream::binary);
	stream.mStream.seekg(file->position);
	stream.mHeader_offset = mHeader.get_header_size();
	return stream;
}
