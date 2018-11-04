#include <wge/filesystem/path.hpp>

namespace wge::filesystem
{

path::path(const char * pString)
{
	parse(pString);
}

path::path(const std::string & pString)
{
	parse(pString);
}

path::path(const system_fs::path & pPath)
{
	for (auto& i : pPath)
		mPath.push_back(i.string());
	simplify();
}

bool path::parse(const std::string & pString, const std::set<char>& pSeparators)
{
	if (pString.empty())
		return false;

	const auto separator_checker = [&pSeparators](const char& l)->bool
	{
		return pSeparators.find(l) != pSeparators.end();
	};

	mPath.clear();

	auto segment_begin_iter = pString.begin();
	auto segment_end_iter = pString.end();
	do {
		// Find the next separator
		segment_end_iter = std::find_if(segment_begin_iter, pString.end(), separator_checker);

		// Add the new segment to the path
		mPath.emplace_back(segment_begin_iter, segment_end_iter);

		// Stop if we reached the end
		if (segment_end_iter == pString.end())
			break;

		// Start the next segment
		segment_begin_iter = segment_end_iter + 1; // "+ 1" to skip the delimitor

	// This condition is false when there is a separator at the end e.g. "dir/"
	} while (segment_begin_iter != pString.end());
	simplify();
	return true;
}


bool path::in_directory(const path & pPath) const
{
	if (pPath.mPath.size() >= mPath.size()) // too big
		return false;
	if (mPath[pPath.mPath.size() - 1] != pPath.filename())
		return false;
	for (size_t i = 0; i < pPath.mPath.size(); i++)
		if (mPath[i] != pPath.mPath[i])
			return false;
	return true;
}

path path::subpath(std::size_t pOffset, std::size_t pCount) const
{
	// Invalid offset
	if (pOffset >= mPath.size())
		return{};

	path result;
	if (pOffset + pCount >= mPath.size() || pCount == 0)
		result.mPath = container(mPath.begin() + pOffset, mPath.end());
	else
		result.mPath = container(mPath.begin() + pOffset, mPath.begin() + pOffset + pCount);
	return result;
}

std::string path::string() const
{
	return string('/'); // Supported by most operating systems
}

std::string path::string(char pSeperator) const
{
	if (mPath.empty())
		return{};
	std::string retval;
	for (auto i : mPath)
		retval += i + pSeperator;
	retval.pop_back(); // Remove the last divider
	return retval;
}

std::string path::stem() const
{
	const std::string name = filename();
	for (auto i = name.begin(); i != name.end(); i++)
		if (*i == '.')
			return std::string(name.begin(), i);
	return name;
}

std::string path::extension() const
{
	const std::string name = filename();
	if (name.empty())
		return{};
	for (auto i = name.rbegin(); i != name.rend(); i++)
		if (*i == '.')
			return std::string(i.base() - 1, name.rbegin().base()); // Include the '.'
	return{};
}

bool path::empty() const
{
	return mPath.empty();
}

void path::clear()
{
	mPath.clear();
}

bool path::is_same(const path & pPath) const
{
	if (pPath.mPath.size() != mPath.size())
		return false;
	if (pPath.filename() != filename())
		return false;
	for (size_t i = 0; i < mPath.size() - 1; i++) // -1 because we already checked for filename
		if (pPath.mPath[i] != mPath[i])
			return false;
	return true;
}

void path::append(const path & pRight)
{
	mPath.insert(mPath.end(), pRight.mPath.begin(), pRight.mPath.end());
	simplify();
}

path path::parent() const
{
	path retval(*this);
	retval.pop_filepath();
	return retval;
}

std::string path::filename() const
{
	if (mPath.empty())
		return{};
	return mPath.back();
}

std::string path::pop_filepath()
{
	if (mPath.empty())
		return false;
	std::string temp = std::move(mPath.back());
	mPath.pop_back();
	return temp;
}

bool path::remove_extension()
{
	if (mPath.empty())
		return false;
	std::string& fn = mPath.back();
	std::size_t dot_index = fn.find_last_of('.');
	if (dot_index == std::string::npos)
		return false;
	fn = fn.substr(0, dot_index);
	return true;
}

size_t path::size() const
{
	return mPath.size();
}

path& path::operator=(const path& pRight)
{
	mPath = pRight.mPath;
	return *this;
}

bool path::operator==(const path& pRight) const
{
	return is_same(pRight);
}

std::string & path::operator[](size_t pIndex)
{
	return mPath[pIndex];
}

const std::string & path::operator[](size_t pIndex) const
{
	return mPath[pIndex];
}

system_fs::path path::to_system_path() const
{
	system_fs::path path;
	for (const auto& i : mPath)
		path /= i;
	return path;
}

path::operator system_fs::path() const
{
	return to_system_path();
}

path::iterator path::begin()
{
	return mPath.begin();
}

path::const_iterator path::begin() const
{
	return mPath.begin();
}

path::iterator path::end()
{
	return mPath.end();
}

path::const_iterator path::end() const
{
	return mPath.end();
}

path path::operator/(const path& pRight) const
{
	path retval(*this);
	retval.append(pRight);
	return retval;
}

path& path::operator/=(const path& pRight)
{
	append(pRight);
	return *this;
}

int path::compare(const path & pCmp) const
{
	// Compare sizes first
	if (mPath.size() == pCmp.mPath.size())
	{
		// Compare the strings
		for (size_t i = 0; i < mPath.size(); i++)
		{
			const int cmp = mPath[i].compare(pCmp.mPath[i]);
			if (cmp != 0)
				return cmp;
		}
	}
	return static_cast<int>(static_cast<long long>(mPath.size())
		- static_cast<long long>(pCmp.mPath.size()));
}

bool path::operator<(const path & pRight) const
{
	return compare(pRight) < 0;
}


void path::simplify()
{
	for (size_t i = 1; i < mPath.size(); i++)
	{
		if (mPath[i].empty() ||
			mPath[i] == ".") // These are redundant
		{
			mPath.erase(mPath.begin() + i);
			--i;
		}
		else if (mPath[i] == "..")
		{
			if (mPath[i - 1] != "..") // Keep these stacked at the beginning
			{
				// ...otherwise, remove this item and the one before it.
				mPath.erase(mPath.begin() + i - 1, mPath.begin() + i + 1);
				i -= 2;
			}
		}
	}
}

void path::push_front(const std::string & pStr)
{
	mPath.push_front(pStr);
}

void path::erase(iterator pBegin, iterator pEnd)
{
	mPath.erase(pBegin, pEnd);
}

path wge::filesystem::make_relative_to(const path & pFrom_directory, const path & pTarget_file)
{
	if (!pTarget_file.in_directory(pFrom_directory))
		return {};

	path temp(pTarget_file);
	temp.erase(temp.begin(), temp.begin() + pFrom_directory.size());
	return temp;
}

} // namespace wge::filesystem
