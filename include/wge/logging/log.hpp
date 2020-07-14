#pragma once

#include <fmt/format.h>
#include <wge/util/span.hpp>

#include <cassert>
#include <any>
#include <string_view>
#include <ctime>

namespace wge::log
{

// Type used to store userdata
using userdata_t = std::any;

// Represents the severity level of a message
enum class level
{
	unknown,
	info,
	debug,
	warning,
	error,
};

// Use WGE_LI macro to generate a line_info
// object based on the current source.
struct line_info
{
	int line{ -1 };
	int column{ -1 };
	std::string file;
	bool system_filesystem{ true };

	void shorten_path();
	std::string to_string() const;
};

struct message
{
	std::string string;
	level severity_level{ level::unknown };
	line_info line_info;
	std::time_t time_stamp{ 0 };

	userdata_t userdata;

	message& with_userdata(userdata_t pUserdata)
	{
		userdata = std::move(pUserdata);
		return *this;
	}

	message& in_file(std::string_view pFile)
	{
		line_info.file = pFile;
		return *this;
	}

	message& at_line(int pLine) noexcept
	{
		line_info.line = pLine;
		return *this;
	}

	message& at_column(int pCol) noexcept
	{
		line_info.column = pCol;
		return *this;
	}

	void stamp_time();
	std::string to_string(bool pAnsi_color = false) const;
};

util::span<const message> get_log();
message& add_message(message&& pMessage);
message& add_message(const message& pMessage);

// Add userdata to the last message.
void userdata(userdata_t pData);

// Returns true if the file was successfully opened
bool open_file(const char* pFile);

// Prints an assertion message as a warning. Returns the boolean result of the expression.
bool soft_assert(bool pExpression, std::string_view pMessage, line_info);

template <typename Tformat, typename...Targs>
inline message& print(level pLevel, const Tformat& pFormat, Targs&&...pArgs)
{
	message msg;
	msg.severity_level = pLevel;
	msg.string = fmt::format(pFormat, std::forward<Targs>(pArgs)...);
	msg.stamp_time();
	return add_message(std::move(msg));
}

template <typename Tformat, typename...Targs>
inline message& info(const Tformat& pFormat, Targs&&...pArgs)
{
	return print(level::info, pFormat, std::forward<Targs>(pArgs)...);
}

template <typename Tformat, typename...Targs>
inline message& debug(const Tformat& pFormat, Targs&&...pArgs)
{
	return print(level::debug, pFormat, std::forward<Targs>(pArgs)...);
}

template <typename Tformat, typename...Targs>
inline message& warning(const Tformat& pFormat, Targs&&...pArgs)
{
	return print(level::warning, pFormat, std::forward<Targs>(pArgs)...);
}

template <typename Tformat, typename...Targs>
inline message& error(const Tformat& pFormat, Targs&&...pArgs)
{
	return print(level::error, pFormat, std::forward<Targs>(pArgs)...);
}

} // namespace wge::log

// Strict assert macro.
#define WGE_ASSERT(A) assert(A)

#define WGE_SASSERT_MSG(Expression, Message) wge::log::soft_assert(Expression, #Expression " - " Message, WGE_LI);
#define WGE_SASSERT(Expression) wge::log::soft_assert(Expression, #Expression, WGE_LI);