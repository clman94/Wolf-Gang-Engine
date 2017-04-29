#include <istream>
#include <fstream>
#include <engine/filesystem.hpp>
#include <engine/utility.hpp>
#include <engine/resource_pack.hpp>

using namespace engine;

// Read an unsigned integer value (of any size) from a stream.
// Format is little endian (for convenience)
template<typename T>
inline T read_unsignedint_binary(std::istream& pStream)
{
	uint8_t bytes[sizeof(T)];// char gives negative values and causes issues when converting to unsigned.
	if (!pStream.read((char*)&bytes, sizeof(T))) // And passing this unsigned char array as a char* works well.
		return 0;

	T val = 0;
	for (size_t i = 0; i < sizeof(T); i++)
		val += static_cast<T>(bytes[i]) << (8 * i);
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

	<whitelist or blacklist>
	<path relative to this text file>
	 ...
	*/

	packing_ignore()
	{
		mIs_valid = false;
	}

	bool open(const encoded_path& pPath, bool pIgnore_self = true)
	{
		mIs_valid = false;
		std::ifstream stream(pPath.string().c_str());
		if (!stream)
			return false;

		// Check if its a blacklist or a whitelist
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

		// Iterate through each line and create an entry for each
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

		// Add entry so it can ignore itself when validating files
		if (pIgnore_self && !mIs_whitelist)
		{
			entry self_entry;
			self_entry.is_directory = false;
			self_entry.path = pPath;
			mFiles.push_back(std::move(self_entry));
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
			if ((!i.is_directory && i.path == pPath) // Path has to be exact when a file
				|| (i.is_directory && pPath.in_directory(i.path))) // File has to be within the directory
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
	const int max_read = 1024;

	auto end = pSrc.tellg();
	pSrc.seekg(0);
	while (pSrc.good() && pDest)
	{
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

bool engine::create_resource_pack(const std::string& pSrc_directory, const std::string& pDest)
{
	const encoded_path root_dir(engine::fs::absolute(pSrc_directory).string());

	packing_ignore ignore_list;

	const std::string ignorelist_path = engine::fs::absolute((root_dir / "pack_ignore.txt").string()).string();
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
	for (auto& i : engine::fs::recursive_directory_iterator(pSrc_directory))
	{
		auto absolute = engine::fs::absolute(i.path());
		if (!engine::fs::is_directory(absolute)
			&& (!ignore_list.is_valid() || ignore_list.is_file_valid(absolute.string())))
			files_to_add.push_back(absolute);
	}

	// Create header
	pack_header header;
	uint64_t current_position = 0;
	for (auto& i : files_to_add)
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
	for (auto& i : files_to_add)
	{
		std::ifstream file_stream(i.string().c_str(), std::fstream::binary | std::fstream::ate);
		if (!file_stream)
			continue;
		util::info("Packing file '" + i.string() + "'...");
		append_stream(stream, file_stream);
	}

	return true;
}

encoded_path::encoded_path(const char * pString)
{
	parse(pString);
}

encoded_path::encoded_path(const std::string & pString)
{
	parse(pString);
}

bool encoded_path::parse(const std::string & pString)
{
	if (pString.empty())
		return false;
	mHierarchy.clear();
	auto start_segment = pString.begin();
	auto end_segment = pString.begin();
	for (; end_segment != pString.end(); end_segment++)
	{
		if (*end_segment == '\\' ||
			*end_segment == '/')
		{
			if (start_segment < end_segment - 1) // Segment is not empty
			{
				mHierarchy.push_back(std::string(start_segment, end_segment));
			}
			start_segment = end_segment + 1;
		}
	}
	if (start_segment < end_segment - 1) // Last segment is not empty
	{
		mHierarchy.push_back(std::string(start_segment, end_segment)); // Get last segment
	}
	simplify();
	return true;
}


bool encoded_path::in_directory(const encoded_path & pPath) const
{
	if (pPath.mHierarchy.size() >= mHierarchy.size())
		return false;
	if (mHierarchy[pPath.mHierarchy.size() - 1] != pPath.filename())
		return false;

	
	for (size_t i = 0; i < pPath.mHierarchy.size(); i++)
	{
		if (mHierarchy[i] != pPath.mHierarchy[i])
			return false;
	}
	return true;
}

bool encoded_path::snip_path(const encoded_path & pPath)
{
	if (!in_directory(pPath))
		return false;
	mHierarchy.erase(mHierarchy.begin()
		, mHierarchy.begin() + pPath.mHierarchy.size());
	return true;
}

std::string encoded_path::string() const
{
	if (mHierarchy.empty())
		return{};
	std::string retval;
	for (auto i : mHierarchy)
	{
		retval += i + "/"; // Supported by windows and linux, however may still not be entirely portable.
						   // TODO: Add preprocessor directives to check for type of system
	}
	retval.pop_back(); // Remove the last divider
	return retval;
}

std::string encoded_path::stem() const
{
	const std::string name = filename();
	auto end = name.begin();
	for (; end != name.end(); end++)
		if (*end == '.')
			return std::string(name.begin(), end);
	return{};
}

std::string encoded_path::extension() const
{
	const std::string name = filename();
	auto end = name.rbegin();
	for (; end != name.rend(); end++)
		if (*end == '.')
			return std::string(end.base() - 1, (name.rbegin()).base()); // Include the '.'
	return{};
}

bool encoded_path::empty() const
{
	return mHierarchy.empty();
}

void encoded_path::clear()
{
	mHierarchy.clear();
}

bool encoded_path::is_same(const encoded_path & pPath) const
{
	if (pPath.mHierarchy.size() != mHierarchy.size())
		return false;
	if (pPath.filename() != filename())
		return false;
	for (size_t i = 0; i < mHierarchy.size() - 1; i++) // -1 because we already checked for filename
		if (pPath.mHierarchy[i] != mHierarchy[i])
			return false;
	return true;
}

void encoded_path::append(const encoded_path & pRight)
{
	mHierarchy.insert(mHierarchy.end(), pRight.mHierarchy.begin(), pRight.mHierarchy.end());
	simplify();
}

encoded_path encoded_path::parent() const
{
	encoded_path retval(*this);
	retval.pop_filename();
	return retval;
}

std::string encoded_path::filename() const
{
	if (mHierarchy.empty())
		return{};
	return mHierarchy.back();
}

bool encoded_path::pop_filename()
{
	if (mHierarchy.empty())
		return false;
	mHierarchy.pop_back();
	return true;
}

encoded_path& encoded_path::operator=(const std::string& pString)
{
	parse(pString);
	return *this;
}

bool encoded_path::operator==(const encoded_path& pRight) const
{
	return is_same(pRight);
}

encoded_path encoded_path::operator/(const encoded_path& pRight) const
{
	encoded_path retval(*this);
	retval.append(pRight);
	return retval;
}

encoded_path& encoded_path::operator/=(const encoded_path& pRight)
{
	append(pRight);
	return *this;
}


void encoded_path::simplify()
{
	for (size_t i = 0; i < mHierarchy.size(); i++)
	{
		if (mHierarchy[i] == ".") // These are redundant
		{
			mHierarchy.erase(mHierarchy.begin() + i);
			--i;
		}
		else if (mHierarchy[i] == "..")
		{
			if (i != 0 && mHierarchy[i - 1] != "..") // Keep these things stacked
			{
				mHierarchy.erase(mHierarchy.begin() + i - 1, mHierarchy.begin() + i + 1);
				i -= 2;
			}
		}
	}
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
	write_unsignedint_binary<uint64_t>(pStream, end - start - 1); // Fill in placeholder
	pStream.seekp(end);
	return true;
}

bool pack_header::parse(std::istream & pStream)
{
	mFiles.clear();

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
		mFiles.push_back(file);
	}
	mHeader_size = pStream.tellg();
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

std::vector<encoded_path> pack_header::recursive_directory(const encoded_path & pPath) const
{
	std::vector<encoded_path> retval;
	for (auto& i : mFiles)
	{
		if (i.path.in_directory(pPath))
			retval.push_back(i.path);
	}
	return retval;
}

uint64_t pack_header::get_header_size() const
{
	return mHeader_size;
}

pack_stream::pack_stream()
{
}

pack_stream::pack_stream(const pack_stream & pCopy)
{
	mHeader_offset = pCopy.mHeader_offset;
	mFile = pCopy.mFile;
	mPack_path = pCopy.mPack_path;
}

void pack_stream::open()
{
	close();
	mStream.open(mPack_path.string().c_str(), std::fstream::binary);
	mStream.seekg(mFile.position + mHeader_offset);
}

void pack_stream::close()
{
	if (mStream.is_open())
		mStream.close();
}

std::vector<char> pack_stream::read(uint64_t pCount)
{
	if (!is_valid())
		return{};

	if (pCount == 0)
		return{};

	std::vector<char> retval;
	retval.resize(static_cast<size_t>(pCount));
	mStream.read(&retval[0], static_cast<size_t>(pCount));
	return retval;
}

int64_t pack_stream::read(char * pData, uint64_t pCount)
{
	if (!is_valid() && pCount == 0)
		return -1;

	// Check bounds
	uint64_t remaining = mFile.size - tell();
	if (remaining < pCount)
	{
		mStream.read(pData, remaining);
		return (int64_t)remaining;
	}
	mStream.read(pData, pCount);
	return (int64_t)pCount;
}

bool pack_stream::read(std::vector<char>& pData, uint64_t pCount)
{
	if (pData.size() != pCount
		|| pCount == 0)
		return false;
	return read(&pData[0], pCount) > 0;
}

std::vector<char> pack_stream::read_all()
{
	const uint64_t chuck_size = 1024;

	seek(0);

	std::vector<char> retval;
	retval.reserve(static_cast<size_t>(mFile.size));
	while (is_valid())
	{
		if (tell() + chuck_size < mFile.size) // Full chunk
		{
			const std::vector<char> data = read(chuck_size);
			if (data.empty())
				return{};
			retval.insert(retval.end(), data.begin(), data.end());
		}
		else // Remainder
		{
			const std::vector<char> data = read(mFile.size - tell());
			retval.insert(retval.end(), data.begin(), data.end());
			return retval;
		}
	}
	return retval;
}


bool pack_stream::seek(uint64_t pPosition)
{
	if (mStream.eof())
		mStream.clear();
	else if (!is_valid())
		return false;

	if (pPosition >= mFile.size)
		return false;
	mStream.seekg(mFile.position + pPosition + mHeader_offset);
	return true;
}

uint64_t pack_stream::tell()
{
	if (!is_valid())
		return 0;

	return (uint64_t)mStream.tellg() - mFile.position - mHeader_offset;
}

bool pack_stream::is_valid()
{
	return mStream.good() 
		&& (uint64_t)mStream.tellg() - mFile.position - mHeader_offset
			< mFile.position + mFile.size;
}

uint64_t pack_stream::size() const
{
	return mFile.size;
}

pack_stream & pack_stream::operator=(const pack_stream & pRight)
{
	close();
	mHeader_offset = pRight.mHeader_offset;
	mFile = pRight.mFile;
	mPack_path = pRight.mPack_path;
	return *this;
}


bool pack_stream_factory::open(const encoded_path& pPath)
{
	std::ifstream stream(pPath.string().c_str(), std::fstream::binary);
	if (!stream)
		return false;
	mPath = pPath;
	return mHeader.parse(stream);
}

pack_stream pack_stream_factory::create_stream(const encoded_path & pPath) const
{
	auto file = mHeader.get_file(pPath);
	if (!file)
		return{};
	pack_stream stream;
	stream.mFile = *file;
	stream.mHeader_offset = mHeader.get_header_size();
	stream.mPack_path = mPath;
	return stream;
}

std::vector<char> engine::pack_stream_factory::read_all(const encoded_path & pPath) const
{
	auto stream = create_stream(pPath);
	stream.open();
	return stream.read_all();
}

std::vector<encoded_path> pack_stream_factory::recursive_directory(const encoded_path & pPath) const
{
	return mHeader.recursive_directory(pPath);
}
