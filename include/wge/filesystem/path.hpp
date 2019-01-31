#pragma once

#include <string>
#include <set>
#include <deque>

#include <filesystem>
namespace system_fs = std::filesystem;

#include <wge/util/hash.hpp>

namespace wge::filesystem
{


// An filepath-like class that doesn't associate with any one filesystem.
// This has some inpiration from the boost::filesystem::path.
class path
{
public:
	typedef std::deque<std::string> container;
	typedef container::iterator iterator;
	typedef container::const_iterator const_iterator;

	path() {}
	path(const char* pString);
	path(const std::string& pString);
	path(const system_fs::path& pPath);

	// Parse a filepath. Returns true if successful.
	bool parse(const std::string& pString, const std::set<char>& pSeparators = { '\\', '/' });

	// Check if first part of this path is the same.
	// E.g. "dir/file.exe" in_directory "dir" => True
	bool in_directory(const path& pPath) const;
	
	path subpath(std::size_t pOffset, std::size_t pCount = 0) const;

	// Convert to string using the '/' separator
	std::string string() const;
	// Convert to string with custom separator
	std::string string(char pSeperator) const;
	// Get the stem of the filename. E.g "dir/file.exe" => "file"
	std::string stem() const;
	// Get the extension of the filename. E.g. "dir/file.exe" => ".exe"
	std::string extension() const;

	// Check if this path is empty
	bool empty() const;

	// Clear this path
	void clear();

	// Check if this path is the same as another
	bool is_same(const path& pPath) const;

	// Add a path to the end of this one. E.g  "dir" + "file.exe" => "dir/file.exe"
	void append(const path& pRight);

	// Get the parent directory of this path. E.g  "dir/file.exe" => "dir"
	path parent() const;

	// Get the file name. E.g  "dir/file.exe" => "file.exe"
	std::string filename() const;

	// Pop the top item. E.g  "dir/file.exe" => "dir"
	std::string pop_filepath();

	// Removed the extension.
	// Returns true if an extension was removed.
	bool remove_extension();

	// Get size of path (amount of items)
	std::size_t size() const;

	path& operator=(const path& pRight);

	// Append this path
	path& operator/=(const path& pRight);
	// Append and return a new path
	path operator/(const path& pRight) const;

	int compare(const path& pCmp) const;
	bool operator<(const path& pRight) const;
	bool operator==(const path& pRight) const;

	std::string& operator[](std::size_t pIndex);
	const std::string& operator[](std::size_t pIndex) const;

	system_fs::path to_system_path() const;
	operator system_fs::path() const;

	// Get the begin iterator of the underlying array
	iterator begin();
	const_iterator begin() const;

	// Get the end iterator of the underlying array
	iterator end();
	const_iterator end() const;

	void push_front(const std::string& pStr);
	void push_back(const std::string& pStr);
	void erase(iterator pBegin, iterator pEnd);

private:
	void simplify();

private:
	container mPath;
};

// Make a relative path from a parent directory to this file.
// Note: This only works if the this file is in the directory.
// make_relative_to("dir/dir1", "dir/dir1/dir2/file.exe") => "dir2/file.exe"
// make_relative_to("dir", "dir2/file.exe") => "" Cannot resolve relative path
path make_relative_to(const path& pFrom_directory, const path& pTarget_file);

}