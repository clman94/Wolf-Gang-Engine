#ifndef ENGINE_RESOURCE_PACK_HPP
#define ENGINE_RESOURCE_PACK_HPP

#include <string>


class encoded_path
{
public:
	encoded_path() {}
	encoded_path(const std::string& pString);

	bool parse(const std::string& pString);

	encoded_path& operator=(const std::string& pString)
	{
		parse(pString);
		return *this;
	}

	// Checks if the path is the same as the parent directory in this path
	bool in_directory(const encoded_path& pPath) const;

	// Check if first part of this path is the same
	bool has_directory(const encoded_path& pPath) const;

	// Snip the first part of this path (if it can)
	bool snip_path(const encoded_path& pPath);

	std::string string() const;

	bool is_same(const encoded_path& pPath) const;

	void append(const encoded_path& pRight);

	encoded_path parent() const
	{
		encoded_path retval(*this);
		retval.pop_filename();
		return retval;
	}

	bool operator==(const encoded_path& pRight) const
	{
		return is_same(pRight);
	}

	encoded_path operator/(const encoded_path& pRight) const
	{
		encoded_path retval(*this);
		retval.append(pRight);
		return retval;
	}

	encoded_path& operator/=(const encoded_path& pRight)
	{
		append(pRight);
		return *this;
	}

	bool pop_filename();

private:
	std::string mFilename;
	std::vector<std::string> mParent_directories;
};

class pack_header
{
public:

	/*
	Structure of the header

	[uint64_t] Size of header (in bytes)
	[uint64_t] File count
	file
	[uint16_t] Path size
	NOTE:  Definitly a loss of data but I've personally never
	found a filepath that is more than 65536 characters
	anywhere near useful.
	[char] Character of path
	...
	[uint64_t] Position
	[uint64_t] Size
	...
	*/

	struct file_info
	{
		encoded_path path;
		uint64_t position;
		uint64_t size;
	};

	void add_file(file_info pFile);

	bool generate(std::ostream& pStream) const;

	bool parse(std::istream& pStream);

	util::optional<file_info> get_file(const encoded_path& pPath) const;

	uint64_t get_header_size() const;

private:
	uint64_t mHeader_size;
	std::vector<file_info> mFiles;
};

class pack_stream
{
public:
	std::vector<char> read(uint64_t pCount);

	bool read(char* pData, uint64_t pCount);
	
	bool seek(uint64_t pPosition);

	uint64_t tell();

	bool is_valid();

	// Quickly open pack and extract the file position
	// NOTE: Header needs to be read EVERYTIME you need to fetch a file. Use sparingly.
	bool open(const std::string& pPack, const std::string& pFile);

	friend class pack_stream_factory;

private:
	uint64_t mHeader_offset;
	pack_header::file_info mFile;
	std::ifstream mStream;
};

// Reads the header of a pack file and generates streams for the contained files
class pack_stream_factory
{
public:
	bool open(const std::string& pPath);

	pack_stream open_file(const std::string& pPath);
private:
	std::string mPath;
	pack_header mHeader;
};

bool create_resource_pack(const std::string& pSrc_directory, const std::string& pDest);

#endif // ENGINE_RESOURCE_PACK_HPP