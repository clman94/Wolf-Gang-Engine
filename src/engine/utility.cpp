#include <engine/utility.hpp>
#include <algorithm>

std::string util::remove_trailing_whitespace(const std::string& pString)
{
	std::string modified(pString);
	modified.erase(modified.begin(), std::find_if_not(modified.begin(), modified.end(), ::isspace));
	modified = std::string(modified.begin(), std::find_if_not(modified.rbegin(), modified.rend(), ::isspace).base());
	return modified;
}