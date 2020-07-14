#include <wge/logging/log.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <iostream>
#include <fstream>
#include <array>
#include <filesystem>

using namespace wge;

static const char* get_ansi_color_reset()
{
	return "\033[0m";
}

static const char* get_ansi_color_for_level(log::level pSeverity_level)
{
	switch (pSeverity_level)
	{
	case log::level::unknown: return "\033[95m"; break;
	case log::level::info:    return get_ansi_color_reset(); break;
	case log::level::debug:   return "\033[96m"; break;
	case log::level::warning: return "\033[93m"; break;
	case log::level::error:   return "\033[91m"; break;
	default:             return get_ansi_color_reset();
	}
}

namespace wge::log
{

static std::vector<message> gLog;
static std::ofstream gLog_output_file;

void message::stamp_time()
{
	time_stamp = std::time(nullptr);
}

std::string message::to_string(bool pAnsi_color) const
{
	std::tm timeinfo;
#ifdef __linux__
	localtime_r(&time, &timeinfo);
#else
	localtime_s(&timeinfo, &time_stamp);
#endif

	constexpr std::array severity_level_name = { "Unknown", "Info", "Debug", "Warning", "Error" };

	fmt::memory_buffer buffer;
	fmt::format_to(buffer, "[{:%m-%d-%y %T}] {}{}: ",
		timeinfo, pAnsi_color ? get_ansi_color_for_level(severity_level) : "",
		severity_level_name[static_cast<std::size_t>(severity_level)]);

	std::string line_info_str = line_info.to_string();
	if (!line_info_str.empty())
		fmt::format_to(buffer, "{}: ", line_info_str);

	fmt::format_to(buffer, "{}{}", string, pAnsi_color ? get_ansi_color_reset() : "");
	return fmt::to_string(std::move(buffer));
}

util::span<const message> get_log()
{
	return gLog;
}

message& add_message(message&& pMessage)
{
	gLog.emplace_back(std::forward<message>(pMessage));
	std::cout << gLog.back().to_string(true) << std::endl;
	return gLog.back();
}

message& add_message(const message& pMessage)
{
	gLog.push_back(pMessage);
	std::cout << gLog.back().to_string(true) << std::endl;
	return gLog.back();
}

void userdata(userdata_t pData)
{
	if (!gLog.empty())
	{
		gLog.back().userdata = std::move(pData);
	}
}

bool open_file(const char* pFile)
{
	gLog_output_file.open(pFile);
	if (gLog_output_file)
		info("Log: Successfully opened log file \"{}\"", pFile);
	else
		error("Log: Failed to open log file \"{}\"", pFile);
	return gLog_output_file.good();
}

bool soft_assert(bool pExpression, std::string_view pMessage, line_info pLine_info)
{
	if (!pExpression)
	{
		pLine_info.shorten_path();
		warning("{} Assertion Failure: {}", pLine_info.to_string(), pMessage);
	}
	return pExpression;
}


void line_info::shorten_path()
{
	if (!file.empty() &&
		system_filesystem &&
		std::filesystem::exists(file))
	{
		std::filesystem::path relate_path = std::filesystem::relative(file);
		std::string path_str = relate_path.string();
		if (file.length() > path_str.length())
			file = path_str;
	}
}

std::string line_info::to_string() const
{
	fmt::memory_buffer buffer;

	if (!file.empty())
		fmt::format_to(buffer, "{}", file);

	const bool has_line = line >= 0;
	const bool has_column = column >= 0;
	if (has_line && !has_column)
		fmt::format_to(buffer, "({})", line);
	else if (has_column)
		fmt::format_to(buffer, "({}, {})",
			has_line ? std::to_string(line).c_str() : "?",
			column);
	return fmt::to_string(buffer);
}

} // namespace wge::log
