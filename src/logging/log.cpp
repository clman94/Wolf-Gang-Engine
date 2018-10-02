#include <wge/logging/log.hpp>

#include <iostream>
#include <fstream>

#include <filesystem>


using namespace wge;
using namespace wge::log;

static message_builder gMessage_builder;
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

extern message_builder& out = gMessage_builder;

std::string message::to_string(bool pAnsi_color) const
{
	std::ostringstream stream;

	std::tm timeinfo;
#ifdef __linux__
	localtime_r(&time, &timeinfo);
#else
	localtime_s(&timeinfo, &time_stamp);
#endif
	const char* time_format = "%m-%d-%y %T";
	char time_str[18];
	std::strftime(time_str, sizeof(time_str), time_format, &timeinfo);
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

message message_builder::finalize()
{
	mStream.flush();
	// Update the message string
	mMessage.string = mStream.str();
	// Clear the stream
	mStream.str({});
	mStream.clear();

	// Record the time this message was created
	mMessage.time_stamp = std::time(nullptr);

	// If the file path is pointing to a system file,
	// generate a relative path so it doesn't take too much space.
	if (!mMessage.line_info.file.empty() &&
		mMessage.line_info.system_filesystem &&
		std::filesystem::exists(mMessage.line_info.file))
	{
		std::filesystem::path relate_path = std::filesystem::relative(mMessage.line_info.file);
		std::string path_str = relate_path.string();
		if (mMessage.line_info.file.length() > path_str.length())
			mMessage.line_info.file = path_str;
	}

	return std::move(mMessage);
}


const std::vector<message>& get_log()
{
	return gLog;
}

bool open_file(const char * pFile)
{
	gLog_output_file.open(pFile);
	if (gLog_output_file)
		info() << "Log: Successfully opened log file \"" << pFile << "\"" << endm;
	else
		error() << "Log: Failed to open log file \"" << pFile << "\"" << endm;
	return gLog_output_file.good();
}

bool soft_assert(bool pExpression, const char * pMessage, line_info pLine_info)
{
	if (!pExpression)
		warning() << pLine_info << "Assertion Failure: " << pMessage << endm;
	return pExpression;
}

void flush()
{
	const auto& msg = gLog.emplace_back(gMessage_builder.finalize());
	if (gLog_output_file)
		gLog_output_file << msg.to_string() << std::endl;
	std::cout << msg.to_string(true) << std::endl;
}

std::ostream& endm(std::ostream& pO)
{
	flush();
	return pO;
}

message_builder& info()
{
	return gMessage_builder << level::info;
}

message_builder& debug()
{
	return gMessage_builder << level::debug;
}

message_builder& warning()
{
	return gMessage_builder << level::warning;
}

message_builder& error()
{
	return gMessage_builder << level::error;
}

} // namespace wge::log