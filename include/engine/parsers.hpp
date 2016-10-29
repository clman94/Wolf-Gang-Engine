#include <vector>
#include <type_traits>
#include <cassert>
#include <string>
#include "utility.hpp"
#include "vector.hpp"

// A series of random string parsers for random things

namespace parsers
{

static bool is_whitespace(char c)
{
	switch (c)
	{
	case ' ':
	case '\n':
	case '\r':
	case '\t':
		return true;
	}
	return false;
}

static std::string remove_trailing_whitespace(const std::string& pString)
{
	if (pString.empty())
		return std::string();

	auto start = pString.begin();
	for (; start != pString.end(); start++)
	{
		if (!is_whitespace(*start))
			break;
	}
	if (start == pString.end()) // All whitespace, return empty
		return std::string();

	auto end = pString.end() - 1;
	for (; end != pString.begin(); end--)
	{
		if (!is_whitespace(*end))
		{
			++end;
			break;
		}
	}

	return std::string(start, end);
}

static bool is_numeral(char c)
{
	if (c >= '0' && c <= '9')
		return true;
	return false;
}

static bool is_letter(char c)
{
	if (c >= 'a' &&
		c <= 'z' ||
		c >= 'A' &&
		c <= 'Z')
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
engine::rect<T> parse_attribute_rect(const std::string& str)
{
	engine::rect<T> r;
	char c = 0;
	for (auto i = str.begin(); i != str.end(); i++)
	{
		if (*i == '=')
		{
			++i;
			if (c == 'x')
				r.x = util::to_numeral<T>(str, i);
			if (c == 'y')
				r.y = util::to_numeral<T>(str, i);
			if (c == 'w')
				r.w = util::to_numeral<T>(str, i);
			if (c == 'h')
				r.h = util::to_numeral<T>(str, i);
		}
		else
			c = *i;
		if (i == str.end())
			break;
	}
	return r;
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