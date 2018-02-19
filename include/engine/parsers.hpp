#ifndef ENGINE_PARSERS_HPP
#define ENGINE_PARSERS_HPP

#include <vector>
#include <type_traits>
#include <cassert>
#include <string>
#include "utility.hpp"
#include "vector.hpp"
#include "rect.hpp"

#include <algorithm>

// A series of random string parsers for random things

namespace parsers
{

static void word_wrap(std::string& pStr, size_t pLength)
{
	if (pStr.empty() || pLength == 0)
		return;
	size_t last_line = 0;
	size_t last_space = 0;
	for (size_t i = 0; i < pStr.size(); i++)
	{
		if (pStr[i] == '\n')
			last_line = i;

		if (pStr[i] == ' ')
			last_space = i;

		if (last_space != 0 && i - last_line > pLength)
		{
			pStr[last_space] = '\n';
			last_line = last_space;
		}
	}
}

static size_t line_count(const std::string & pStr)
{
	if (pStr.empty())
		return 0;

	size_t line_count = 1;
	for (size_t i = 0; i < pStr.size(); i++)
	{
		if (pStr[i] == '\n')
			++line_count;
	}
	return line_count;
}

static size_t remove_first_line(std::string& pStr)
{
	for (size_t i = 0; i < pStr.size(); i++)
	{
		if (pStr[i] == '\n'
			|| i == pStr.size() - 1)
		{
			pStr = std::string(pStr.begin() + i + 1, pStr.end());
			return i;
		}
	}
	return 0;
}

static size_t limit_lines(std::string& pStr, size_t pLines)
{
	size_t current_line_count = line_count(pStr);
	if (current_line_count > pLines)
	{
		size_t amount_removed = 0;
		for (size_t i = 0; i < current_line_count - pLines; i++)
		{
			amount_removed += remove_first_line(pStr);
		}
		return amount_removed;
	}
	return 0;
}


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


static bool is_numeral(char c)
{
	if (c >= '0' && c <= '9')
		return true;
	return false;
}

static bool is_letter(char c)
{
	if ((c >= 'a' &&
		c <= 'z') ||
		(c >= 'A' &&
		c <= 'Z'))
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

static std::string auto_adjust_precision(const std::string& pStr)
{
	if (pStr.empty())
		return{};
	for (auto i = pStr.rbegin(); i != pStr.rend(); i++)
	{
		if (*i == '.')
		{
			return std::string(pStr.begin(), i.base() - 1);
		}
		if (*i != '0')
		{
			return std::string(pStr.begin(), i.base());
		}
	}
	return{};
}

template<typename T>
static std::string generate_attribute_rect(const engine::rect<T>& pRect)
{
	std::string retval;
	retval = "x=";
	retval += auto_adjust_precision(std::to_string(pRect.x));
	retval += " y=";
	retval += auto_adjust_precision(std::to_string(pRect.y));
	retval += " w=";
	retval += auto_adjust_precision(std::to_string(pRect.w));
	retval += " h=";
	retval += auto_adjust_precision(std::to_string(pRect.h));
	return retval;
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

#endif // !ENGINE_PARSERS_HPP