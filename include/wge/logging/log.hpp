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

	void stamp_time();
	std::string to_string(bool pAnsi_color = false) const;
};

// Adds "something" as userdata in your message.
// This is primarily meant for a custom log UI that
// can display extra information about a message eg
// stack info in a script.
// Usage: log::out << log::userdata(234) << "my message" << log::endm;
struct userdata
{
	template <typename T>
	userdata(T&& pAny) :
		data(std::forward<T>(pAny))
	{}
	userdata_t data;
};

util::span<const message> get_log();
void add_message(message&& pMessage);
void add_message(const message& pMessage);

// Returns true if the file was successfully opened
bool open_file(const char* pFile);

// Prints an assertion message as a warning. Returns the boolean result of the expression.
bool soft_assert(bool pExpression, std::string_view pMessage, line_info);

template <typename Tformat, typename...Targs>
void print(level pLevel, const Tformat& pFormat, Targs&&...pArgs)
{
	message msg;
	msg.severity_level = pLevel;
	msg.string = fmt::format(pFormat, std::forward<Targs>(pArgs)...);
	msg.stamp_time();
	add_message(std::move(msg));
}

template <typename Tformat, typename...Targs>
void info(const Tformat& pFormat, Targs&&...pArgs)
{
	print(level::info, pFormat, std::forward<Targs>(pArgs)...);
}

template <typename Tformat, typename...Targs>
void debug(const Tformat& pFormat, Targs&&...pArgs)
{
	print(level::debug, pFormat, std::forward<Targs>(pArgs)...);
}

template <typename Tformat, typename...Targs>
void warning(const Tformat& pFormat, Targs&&...pArgs)
{
	print(level::warning, pFormat, std::forward<Targs>(pArgs)...);
}

template <typename Tformat, typename...Targs>
void error(const Tformat& pFormat, Targs&&...pArgs)
{
	print(level::error, pFormat, std::forward<Targs>(pArgs)...);
}

} // namespace wge::log

// Strict assert macro.
#define WGE_ASSERT(A) assert(A)

#define WGE_SASSERT_MSG(Expression, Message) wge::log::soft_assert(Expression, #Expression " - " Message, WGE_LI);
#define WGE_SASSERT(Expression) wge::log::soft_assert(Expression, #Expression, WGE_LI);