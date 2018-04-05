#ifndef ENGINE_RESOURCE_PACK_HPP
#define ENGINE_RESOURCE_PACK_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <set>
#include <engine/utility.hpp>
#include <engine/filesystem.hpp>

namespace engine {

// An filepath-like class that doesn't associate with any one filesystem.
class generic_path
{
public:
	generic_path() {}
	generic_path(const char* pString);
	generic_path(const std::string& pString);
	generic_path(const fs::path& pPath);

	bool parse(const std::string& pString, const std::set<char>& pDelimitors = {'\\', '/'});

	// Check if first part of this path is the same
	bool in_directory(const generic_path& pPath) const;

	// Snip the first part of this path (if it can)
	bool snip_path(const generic_path& pPath);

	generic_path subpath(size_t pOffset, size_t pCount = 0) const;

	std::string string() const;
	std::string string(char pSeperator) const;
	std::string stem() const;
	std::string extension() const;

	bool empty() const;

	void clear();

	bool is_same(const generic_path& pPath) const;

	void append(const generic_path& pRight);

	generic_path parent() const;

	std::string filename() const;
	bool pop_filename();

	bool remove_extension();

	size_t get_sub_length() const;

	generic_path& operator=(const std::string& pString);
	generic_path operator/(const generic_path& pRight) const;
	generic_path& operator/=(const generic_path& pRight);

	int compare(const generic_path& pCmp) const;
	bool operator<(const generic_path& pRight) const;
	bool operator==(const generic_path& pRight) const;

	std::string& operator[](size_t pIndex);
	const std::string& operator[](size_t pIndex) const;

	fs::path to_path() const;

	std::vector<std::string>::iterator begin();
	std::vector<std::string>::const_iterator begin() const;

	std::vector<std::string>::iterator end();
	std::vector<std::string>::const_iterator end() const;

private:
	void simplify();

	std::vector<std::string> mHierarchy;
};

class pack_header
{
public:

	/*
	Structure of the header

	[uint64_t] File count
	file
	[uint16_t] Path size (Is a filepath that is 65536 characters useful?)
	[char] Character of path
	...
	[uint64_t] Position
	[uint64_t] Size
	...
	*/

	static_assert(sizeof(uint16_t) == 2, "uint16_t is not 2 bytes");
	static_assert(sizeof(uint64_t) == 8, "uint64_t is not 8 bytes");

	pack_header();

	struct file_info
	{
		generic_path path;
		uint64_t position;
		uint64_t size;
	};

	void add_file(file_info pFile);

	bool generate(std::ostream& pStream) const;

	bool parse(std::istream& pStream);

	util::optional<file_info> get_file(const generic_path& pPath) const;
	std::vector<generic_path> recursive_directory(const generic_path& pPath) const;

	uint64_t get_header_size() const;

private:
	uint64_t mHeader_size;
	std::vector<file_info> mFiles;
};

// Reads the header of a pack file and generates streams for the contained files
class resource_pack
{
public:
	bool open(const generic_path& pPath);
	std::vector<char> read_all(const generic_path& pPath) const;
	std::vector<generic_path> recursive_directory(const generic_path& pPath) const;
private:
	generic_path mPath;
	pack_header mHeader;
	friend class pack_stream;
};

class pack_stream
{
public:
	pack_stream();
	pack_stream(const resource_pack& pPack);
	pack_stream(const resource_pack& pPack, const generic_path & pPath);
	pack_stream(const pack_stream& pCopy);
	~pack_stream();

	void set_pack(const resource_pack& pPack);

	bool open(const generic_path & pPath);
	void close();

	std::vector<char> read(uint64_t pCount);
	int64_t read(char* pData, uint64_t pCount);
	bool read(std::vector<char>& pData, uint64_t pCount);

	std::vector<char> read_all();

	bool seek(uint64_t pPosition);
	uint64_t tell();

	bool is_valid();

	uint64_t size() const;

	pack_stream& operator=(const pack_stream& pRight);

	operator bool() { return is_valid(); }

	friend class resource_pack;

private:
	const resource_pack * mPack;
	pack_header::file_info mFile_info;
	std::ifstream mStream;
};

bool create_resource_pack(const std::string& pSrc_directory, const std::string& pDest);

}
#endif // ENGINE_RESOURCE_PACK_HPP