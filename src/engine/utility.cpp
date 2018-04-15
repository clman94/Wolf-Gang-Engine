#include <engine/utility.hpp>
#include <algorithm>
#include <fstream>

std::string util::remove_trailing_whitespace(const std::string& pString)
{
	std::string modified(pString);
	modified.erase(modified.begin(), std::find_if_not(modified.begin(), modified.end(), ::isspace));
	modified = std::string(modified.begin(), std::find_if_not(modified.rbegin(), modified.rend(), ::isspace).base());
	return modified;
}

bool util::replace_all_with(std::string & pVal, const std::string & pTarget, const std::string & pNew)
{
	if (pTarget.length() > pVal.length()
		|| pTarget.empty() || pVal.empty())
		return false;
	bool ret = false;
	for (size_t i = 0; i < pVal.length() - pTarget.length() + 1; i++)
	{
		if (std::string(pVal.begin() + i, pVal.begin() + i + pTarget.size()) == pTarget) // Compare this range
		{
			pVal.replace(i, pTarget.length(), pNew); // Replace
			i += pNew.length() - 1; // Skip the new text
			ret = true;
		}
	}
	return ret;
}

bool util::text_file_readall(const std::string & pPath, std::string & pOut)
{
	std::string xml_default_str;
	std::ifstream stream(pPath.c_str());
	if (!stream)
		return false;

	// Allocate
	stream.seekg(0, std::ios::end);
	pOut.reserve(stream.tellg());
	stream.seekg(0, std::ios::beg);

	// Read all of it
	pOut.assign((std::istreambuf_iterator<char>(stream)),
	             std::istreambuf_iterator<char>());

	stream.close();
	return true;
}
