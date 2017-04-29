#ifndef ENGINE_RESOURCE_PACK_HPP
#define ENGINE_RESOURCE_PACK_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <engine/utility.hpp>

namespace engine {

// A path somewhat similar to boost path but with more features
// related to manipulating the path.
class encoded_path
{
public:
	encoded_path() {}
	encoded_path(const char* pString);
	encoded_path(const std::string& pString);

	bool parse(const std::string& pString);

	// Check if first part of this path is the same
	bool in_directory(const encoded_path& pPath) const;

	// Snip the first part of this path (if it can)
	bool snip_path(const encoded_path& pPath);

	std::string string() const;
	std::string stem() const;
	std::string extension() const;

	bool empty() const;

	void clear();

	bool is_same(const encoded_path& pPath) const;

	void append(const encoded_path& pRight);

	encoded_path parent() const;

	std::string filename() const;

	bool pop_filename();

	encoded_path& operator=(const std::string& pString);
	bool operator==(const encoded_path& pRight) const;
	encoded_path operator/(const encoded_path& pRight) const;
	encoded_path& operator/=(const encoded_path& pRight);

private:
	void simplify();

	std::vector<std::string> mHierarchy;
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
	std::vector<encoded_path> recursive_directory(const encoded_path& pPath) const;

	uint64_t get_header_size() const;

private:
	uint64_t mHeader_size;
	std::vector<file_info> mFiles;
};

class pack_stream
{
public:
	pack_stream();
	pack_stream(const pack_stream& pCopy);

	void open();
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

	friend class pack_stream_factory;

private:
	uint64_t mHeader_offset;
	pack_header::file_info mFile;
	encoded_path mPack_path;
	std::ifstream mStream;
};

// Reads the header of a pack file and generates streams for the contained files
class pack_stream_factory
{
public:
	bool open(const encoded_path& pPath);
	pack_stream create_stream(const encoded_path& pPath) const;
	std::vector<char> read_all(const encoded_path& pPath) const;
	std::vector<encoded_path> recursive_directory(const encoded_path& pPath) const;
private:
	encoded_path mPath;
	pack_header mHeader;
};

bool create_resource_pack(const std::string& pSrc_directory, const std::string& pDest);

}
#endif // ENGINE_RESOURCE_PACK_HPP