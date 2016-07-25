#include <vector>
#include <type_traits>
#include <cassert>
#include <string>
#include "utility.hpp"
#include "vector.hpp"

namespace parsers
{

static bool is_numeral(char c)
{
	if (c >= '0' && c <= '9')
		return true;
	return false;
}

template<typename T>
class range
{
	struct section
	{
		T start, end;
	};
	std::vector<section> sections;
public:
	bool in_range(T a)
	{
		for (auto i : sections)
		{
			if (a >= i.start && a <= i.end)
				return true;
		}
		return false;
	}
	friend range<T> parse_range(const std::string& str);
};

template<typename T>
static range<T> parse_range(const std::string& str)
{
	range<T> retval;

	int c = 0;

	T start = 0;
	T end = 0;

	for (auto i = str.begin(); i != str.end(); i++)
	{
		if (is_numeral(*i))
		{
			start = util::to_numeral<T>(str, i);
			end = start;
		}
		if (*i == '-')
		{
			end = util::to_numeral<T>(str, ++i);
			retval.sections.emplace_back(start, end);
		}
		if (*i == ',')
		{
			start = -1;
		}
		if (i == str.end())
			break;
	}

	return std::move(retval);
}

template<typename T>
static engine::vector<T> parse_vector(const std::string& str)
{
	engine::vector<T> retval;

	auto i = str.begin();
	while (i != str.end())
	{
		if (is_numeral(*i))
		{
			retval.x = util::to_numeral<T>(str, i);
		}
		if (*i == ',')
		{
			retval.y = util::to_numeral<T>(str, ++i);
			break;
		}
		if (i == str.end())
			break;
		++i;
	}
	return retval;
}

}