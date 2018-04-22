#include <istream>
#include <fstream>
#include <engine/filesystem.hpp>
#include <engine/resource_pack.hpp>
#include <engine/logger.hpp>
#include <engine/binary_util.hpp>

using namespace engine;

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

	bool open(const generic_path& pPath, bool pIgnore_self = true)
	{
		mIs_valid = false;
		std::ifstream stream(pPath.string().c_str());
		if (!stream)
			return false;

		// Iterate through each line and create an entry for each
		const generic_path parent_folder(pPath.parent());
		for (std::string line; std::getline(stream, line);)
		{
			const generic_path path(parent_folder / line);
			const std::string path_string = path.string();
			if (!engine::fs::exists(path_string))
			{
				logger::warning("File does not exist '" + path_string + "'");
				continue;
			}

			entry nentry;
			nentry.is_directory = engine::fs::is_directory(path.string());
			nentry.path = path;
			mFiles.push_back(std::move(nentry));
		}

		// Add entry so it can ignore itself when validating files
		if (pIgnore_self)
		{
			entry self_entry;
			self_entry.is_directory = false;
			self_entry.path = pPath;
			mFiles.push_back(std::move(self_entry));
		}

		mIs_valid = true;
		return true;
	}

	bool is_file_valid(const generic_path& pPath) const
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
		return has_entry;
	}

	bool is_valid() const
	{
		return mIs_valid;
	}

private:

	struct entry
	{
		bool is_directory;
		generic_path path;
	};
	bool mIs_valid;
	std::vector<entry> mFiles;
};

inline bool append_stream(std::ostream& pDest, std::istream& pSrc)
{
	const int max_read = 4096;

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
	const generic_path root_dir(engine::fs::absolute(pSrc_directory).string());

	packing_ignore ignore_list;

	const std::string ignorelist_path = engine::fs::absolute((root_dir / "pack_ignore.txt").string()).string();
	if (ignore_list.open(ignorelist_path))
	{
		logger::info("Loaded ignore list '" + ignorelist_path + "'");
	}
	else
	{
		logger::warning("Couldn't load ignore list '" + ignorelist_path + "'");
		logger::info("If you didn't specify a list, you can ignore the previous warning.");
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
		generic_path path(i.string());
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
		{
			logger::error("Failed to pack file '" + i.string() + "'...");
			continue;
		}
		logger::info("Packing file '" + i.string() + "'...");
		append_stream(stream, file_stream); // possibly replace with stream << file_stream.rdbuf();
	}
	return true;
}

generic_path::generic_path(const char * pString)
{
	parse(pString);
}

generic_path::generic_path(const std::string & pString)
{
	parse(pString);
}

generic_path::generic_path(const fs::path & pPath)
{
	for (auto& i : pPath)
		mHierarchy.push_back(i.string());
}

bool generic_path::parse(const std::string & pString, const std::set<char>& pDelimitors)
{
	if (pString.empty())
		return false;
	mHierarchy.clear();
	auto start_segment = pString.begin();
	auto end_segment = pString.begin();
	for (; end_segment != pString.end(); end_segment++)
	{
		if (pDelimitors.find(*end_segment) != pDelimitors.end())
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


bool generic_path::in_directory(const generic_path & pPath) const
{
	if (pPath.mHierarchy.size() >= mHierarchy.size()) // too big
		return false;
	if (mHierarchy[pPath.mHierarchy.size() - 1] != pPath.filename())
		return false;
	for (size_t i = 0; i < pPath.mHierarchy.size(); i++)
		if (mHierarchy[i] != pPath.mHierarchy[i])
			return false;
	return true;
}

bool generic_path::snip_path(const generic_path & pPath)
{
	if (!in_directory(pPath))
		return false;
	mHierarchy.erase(mHierarchy.begin()
		, mHierarchy.begin() + pPath.mHierarchy.size());
	return true;
}

generic_path generic_path::subpath(size_t pOffset, size_t pCount) const
{
	if (pOffset >= mHierarchy.size())
		return{};

	if (pOffset + pCount >= mHierarchy.size() || pCount == 0)
	{
		generic_path new_path;
		new_path.mHierarchy = std::vector<std::string>(mHierarchy.begin() + pOffset, mHierarchy.end());
		return new_path;
	}

	generic_path new_path;
	new_path.mHierarchy 
		= std::vector<std::string>(mHierarchy.begin() + pOffset
			, mHierarchy.begin() + pOffset + pCount);
	return new_path;
}

std::string generic_path::string() const
{
	return string('/'); // Supported by windows and linux, however may still not be entirely portable.
					    // TODO: Add preprocessor directives to check for type of system
}

std::string generic_path::string(char pSeperator) const
{
	if (mHierarchy.empty())
		return{};
	std::string retval;
	for (auto i : mHierarchy)
		retval += i + pSeperator;
	retval.pop_back(); // Remove the last divider
	return retval;
}

std::string generic_path::stem() const
{
	const std::string name = filename();
	for (auto i = name.begin(); i != name.end(); i++)
		if (*i == '.')
			return std::string(name.begin(), i);
	return name;
}

std::string generic_path::extension() const
{
	const std::string name = filename();
	if (name.empty())
		return{};
	for (auto i = name.rbegin(); i != name.rend(); i++)
		if (*i == '.')
			return std::string(i.base() - 1, name.rbegin().base()); // Include the '.'
	return{};
}

bool generic_path::empty() const
{
	return mHierarchy.empty();
}

void generic_path::clear()
{
	mHierarchy.clear();
}

bool generic_path::is_same(const generic_path & pPath) const
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

void generic_path::append(const generic_path & pRight)
{
	mHierarchy.insert(mHierarchy.end(), pRight.mHierarchy.begin(), pRight.mHierarchy.end());
	simplify();
}

generic_path generic_path::parent() const
{
	generic_path retval(*this);
	retval.pop_filename();
	return retval;
}

std::string generic_path::filename() const
{
	if (mHierarchy.empty())
		return{};
	return mHierarchy.back();
}

bool generic_path::pop_filename()
{
	if (mHierarchy.empty())
		return false;
	mHierarchy.pop_back();
	return true;
}

bool generic_path::remove_extension()
{
	if (mHierarchy.empty())
		return false;
	std::string& fn = mHierarchy.back();
	for (auto i = fn.begin(); i != fn.end(); i++)
	{
		if (*i == '.')
		{
			fn = std::string(fn.begin(), i);
			return true;
		}
	}
	return false;
}

size_t generic_path::get_sub_length() const
{
	return mHierarchy.size();
}

generic_path& generic_path::operator=(const std::string& pString)
{
	parse(pString);
	return *this;
}

bool generic_path::operator==(const generic_path& pRight) const
{
	return is_same(pRight);
}

std::string & generic_path::operator[](size_t pIndex)
{
	return mHierarchy[pIndex];
}

const std::string & generic_path::operator[](size_t pIndex) const
{
	return mHierarchy[pIndex];
}

fs::path generic_path::to_path() const
{
	fs::path path;
	for (const auto& i : mHierarchy)
		path /= i;
	return path;
}

std::vector<std::string>::iterator generic_path::begin()
{
	return mHierarchy.begin();
}

std::vector<std::string>::const_iterator generic_path::begin() const
{
	return mHierarchy.begin();
}

std::vector<std::string>::iterator engine::generic_path::end()
{
	return mHierarchy.end();
}

std::vector<std::string>::const_iterator generic_path::end() const
{
	return mHierarchy.end();
}

generic_path generic_path::operator/(const generic_path& pRight) const
{
	generic_path retval(*this);
	retval.append(pRight);
	return retval;
}

generic_path& generic_path::operator/=(const generic_path& pRight)
{
	append(pRight);
	return *this;
}

int generic_path::compare(const generic_path & pCmp) const
{
	if (mHierarchy.size() == pCmp.mHierarchy.size())
	{
		for (size_t i = 0; i < mHierarchy.size(); i++)
		{
			const int cmp = mHierarchy[i].compare(pCmp.mHierarchy[i]);
			if (cmp != 0)
				return cmp;
		}
	}
	return static_cast<int>(static_cast<long long>(mHierarchy.size())
		- static_cast<long long>(pCmp.mHierarchy.size()));
}

bool generic_path::operator<(const generic_path & pRight) const
{
	return compare(pRight) < 0;
}


void generic_path::simplify()
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

pack_header::pack_header()
{
	mHeader_size = 0;
}

void pack_header::add_file(file_info pFile)
{
	mFiles.push_back(pFile);
}

bool pack_header::generate(std::ostream & pStream) const
{
	// File count
	binary_util::write_unsignedint_binary<uint64_t>(pStream, mFiles.size());

	// File list
	for (auto& i : mFiles)
	{
		// Write path string
		std::string path = i.path.string();
		binary_util::write_unsignedint_binary<uint16_t>(pStream, static_cast<uint16_t>(path.size()));
		pStream.write(path.c_str(), path.size());

		binary_util::write_unsignedint_binary<uint64_t>(pStream, i.position);
		binary_util::write_unsignedint_binary<uint64_t>(pStream, i.size);
	}
	return true;
}

bool pack_header::parse(std::istream & pStream)
{
	mFiles.clear();

	// File count
	uint64_t file_count = binary_util::read_unsignedint_binary<uint64_t>(pStream);
	if (file_count == 0)
		return false;

	// File list
	for (uint64_t i = 0; i < file_count; i++)
	{
		file_info file;

		// Read path string
		uint16_t path_size = binary_util::read_unsignedint_binary<uint16_t>(pStream);
		std::string path;
		path.resize(path_size);
		if (!pStream.read(&path[0], path_size))
			return false;

		file.path = path;
		file.position = binary_util::read_unsignedint_binary<uint64_t>(pStream);
		file.size = binary_util::read_unsignedint_binary<uint64_t>(pStream);

		mFiles.push_back(file);
	}

	mHeader_size = pStream.tellg();
	return true;
}

util::optional<pack_header::file_info> pack_header::get_file(const generic_path & pPath) const
{
	for (auto& i : mFiles)
	{
		if (i.path == pPath)
			return i;
	}
	return{};
}

std::vector<generic_path> pack_header::recursive_directory(const generic_path & pPath) const
{
	std::vector<generic_path> retval;
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
	mPack = nullptr;
}

pack_stream::pack_stream(const resource_pack& pPack)
{
	mPack = &pPack;
}

pack_stream::pack_stream(const resource_pack & pPack, const generic_path & pPath)
{
	mPack = &pPack;
	open(pPath);
}

pack_stream::pack_stream(const pack_stream & pCopy)
{
	mPack = pCopy.mPack;
	mFile_info = pCopy.mFile_info;
}

pack_stream::~pack_stream()
{
	close();
}

void pack_stream::set_pack(const resource_pack & pPack)
{
	mPack = &pPack;
}

bool pack_stream::open(const generic_path & pPath)
{
	close();
	mStream.open(mPack->mPath.string().c_str(), std::fstream::binary);
	if (!mStream)
		return false;

	auto fi = mPack->mHeader.get_file(pPath);
	if (!fi)
		return false;
	mFile_info = *fi;

	mStream.seekg(mFile_info.position + mPack->mHeader.get_header_size());

	return is_valid();
}

void pack_stream::close()
{
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
	uint64_t remaining = mFile_info.size - tell();
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
	retval.reserve(static_cast<size_t>(mFile_info.size));
	while (is_valid())
	{
		if (tell() + chuck_size < mFile_info.size) // Full chunk
		{
			const std::vector<char> data = read(chuck_size);
			if (data.empty())
				return{};
			retval.insert(retval.end(), data.begin(), data.end());
		}
		else // Remainder
		{
			const std::vector<char> data = read(mFile_info.size - tell());
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

	if (pPosition >= mFile_info.size)
		return false;
	mStream.seekg(mFile_info.position + pPosition + mPack->mHeader.get_header_size());
	return true;
}

uint64_t pack_stream::tell()
{
	if (!is_valid())
		return 0;

	return (uint64_t)mStream.tellg() - mFile_info.position - mPack->mHeader.get_header_size();
}

bool pack_stream::is_valid()
{
	return mStream.good() 
		&& (uint64_t)mStream.tellg() - mFile_info.position - mPack->mHeader.get_header_size()
			< mFile_info.position + mFile_info.size;
}

uint64_t pack_stream::size() const
{
	return mFile_info.size;
}

pack_stream & pack_stream::operator=(const pack_stream & pRight)
{
	close();
	mPack = pRight.mPack;
	mFile_info = pRight.mFile_info;
	return *this;
}


bool resource_pack::open(const generic_path& pPath)
{
	std::ifstream stream(pPath.string().c_str(), std::fstream::binary);
	if (!stream)
		return false;
	mPath = pPath;
	return mHeader.parse(stream);
}

std::vector<char> resource_pack::read_all(const generic_path & pPath) const
{
	pack_stream stream(*this);
	stream.open(pPath);
	return stream.read_all();
}

std::vector<generic_path> resource_pack::recursive_directory(const generic_path & pPath) const
{
	return mHeader.recursive_directory(pPath);
}
