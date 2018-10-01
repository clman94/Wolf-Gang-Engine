#include <wge/logging/log.hpp>

#include <iostream>
#include <sstream>
#include <fstream>

#include <filesystem>


using namespace wge;
using namespace wge::log;

static log::message gMessage;
static std::ostringstream gOut_stream;
static log_container gLog;
static std::ofstream gLog_output_file;

static const char* get_ansi_color_reset()
{
	return "\033[0m";
}

static const char* get_ansi_color_for_level(log::level pSeverity_level)
{
	switch (pSeverity_level)
	{
	case level::unknown: return "\033[95m"; break;
	case level::info:    return get_ansi_color_reset(); break;
	case level::debug:   return "\033[96m"; break;
	case level::warning: return "\033[93m"; break;
	case level::error:   return "\033[91m"; break;
	default:             return get_ansi_color_reset();
	}
}

namespace wge::log
{

extern log_ostream& out = gOut_stream;

std::string message::to_string(bool pAnsi_color) const
{
	std::ostringstream stream;

	// Generate the time string based on the message's time stamp
	const char* time_format = "%m-%d-%y %T";
	char time_str[18];
	std::strftime(time_str, sizeof(time_str), time_format, std::localtime(&time_stamp));
	stream << '[' << time_str << "] ";

	if (pAnsi_color)
		stream << get_ansi_color_for_level(severity_level);

	switch (severity_level)
	{
	case level::unknown: stream << "Unknown: "; break;
	case level::info:    stream << "Info: "; break;
	case level::debug:   stream << "Debug: "; break;
	case level::warning: stream << "Warning: "; break;
	case level::error:   stream << "Error: "; break;
	}

	if (!line_info.file.empty())
		stream << line_info.file;

	const bool has_line = line_info.line >= 0;
	const bool has_column = line_info.column >= 0;
	if (has_line || has_column)
	{
		stream << " (";
		stream << (has_line ? line_info.line : '?');
		if (has_column)
			stream << ", " << line_info.column;
		stream << ')';
	}

	if (!line_info.file.empty() || has_line || has_column)
		stream << ": ";

	stream << string;

	if (pAnsi_color)
		stream << get_ansi_color_reset();
	return stream.str();
}

} // namespace wge::log

const std::vector<message>& wge::log::get_log()
{
	return gLog;
}

bool wge::log::open_file(const char * pFile)
{
	gLog_output_file.open(pFile);
	if (gLog_output_file)
		info() << "Log: Succussfully opened log file \"" << pFile << "\"" << endm;
	else
		error() << "Log: Failed to open log file \"" << pFile << "\"" << endm;
	return gLog_output_file.good();
}

bool wge::log::soft_assert(bool pExpression, const char * pMessage, line_info pLine_info)
{
	if (!pExpression)
		warning() << pLine_info << "Assertion Failure: " << pMessage << endm;
	return pExpression;
}

void wge::log::flush()
{
	gOut_stream.flush();
	// Update the message string
	gMessage.string = gOut_stream.str();
	// Clear the stream
	gOut_stream.str({});
	gOut_stream.clear();

	// Record the time this message was created
	gMessage.time_stamp = std::time(nullptr);		

	// If the file path is pointing to a system file,
	// generate a relative path so it doesn't take too much space.
	if (!gMessage.line_info.file.empty() && 
		gMessage.line_info.system_filesystem &&
		std::filesystem::exists(gMessage.line_info.file))
	{
		std::filesystem::path relate_path = std::filesystem::relative(gMessage.line_info.file);
		std::string path_str = relate_path.string();
		if (gMessage.line_info.file.length() > path_str.length())
			gMessage.line_info.file = path_str;
	}

	// Get the final message string to print out to
	// the terminal and log file.
	if (gLog_output_file)
		gLog_output_file << gMessage.to_string() << std::endl;
	std::cout << gMessage.to_string(true) << std::endl;
	// Move the message into the container clearing gMessage
	// at the same time.
	std::swap(gLog.emplace_back(), gMessage);
}

log_ostream & wge::log::endm(std::ostream & os)
{
	flush();
	return os;
}

log_ostream& wge::log::operator << (log_ostream& pOs, line_info pLine_info)
{
	gMessage.line_info = pLine_info;
	return pOs;
}

log_ostream& wge::log::operator << (log_ostream& pOs, level pLevel)
{
	gMessage.severity_level = pLevel;
	return pOs;
}

log_ostream & wge::log::info()
{
	return gOut_stream << level::info;
}

log_ostream & wge::log::debug()
{
	return gOut_stream << level::debug;
}

log_ostream & wge::log::warning()
{
	return gOut_stream << level::warning;
}

log_ostream & wge::log::error()
{
	return gOut_stream << level::error;
}
