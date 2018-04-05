#include <engine/filesystem.hpp>
#include <engine/logger.hpp>
#include <fstream>
#include <iostream>
#include <cassert>

// For time
#include <ctime>
#include <iomanip>
#include <chrono>

namespace logger {

static std::string mLog_string;
static std::vector<message> mLog;
static std::ofstream mLog_file;

void initialize(const std::string & pOutput)
{
	if (mLog_file)
		mLog_file.close();

	mLog_file.open(pOutput.c_str());
	if (!mLog_file)
		error("Failed to open log output file");
	else
		info("Log file initialized at '" + pOutput + "'");
}

message print(const message & pMessage)
{
	mLog.push_back(pMessage);

	std::string stringified_message = pMessage.to_string() + "\n";
	mLog_string += stringified_message;

	if (mLog_file)
	{
		mLog_file << stringified_message; // Save to file

		// Ensure the file is updated.
		// Last thing we want is the application to crash
		// and there is nothing the log!
		mLog_file.flush();
	}

	// Printing to the console is disabled to
	// to remove the overhead and the redundancy.
#ifndef LOCKED_RELEASE_MODE
	std::cout << stringified_message; // Print to console  
#endif

	return pMessage;
}

message print(level pType, const std::string& pMessage)
{
	message nmsg;
	nmsg.is_file = false;
	nmsg.type = pType;
	nmsg.msg = pMessage;
	nmsg.set_to_current_time();
	return print(nmsg);
}

message print(const std::string& pFile, int pLine, level pType, const std::string& pMessage)
{
	return print(pFile, pLine, -1, pType, pMessage);
}

message print(const std::string& pFile, int pLine, int pCol, level pType, const std::string& pMessage)
{
	message nmsg;

	nmsg.is_file = true;
	nmsg.file = pFile;
	nmsg.row = pLine;
	nmsg.column = pCol;
	nmsg.type = pType;
	nmsg.msg = pMessage;
	nmsg.set_to_current_time();
	return print(nmsg);
}

void error(const std::string& pMessage)
{
	print(level::error, pMessage);
}

void warning(const std::string& pMessage)
{
	print(level::warning, pMessage);
}

void info(const std::string& pMessage)
{
	print(level::info, pMessage);
}

const std::vector<message>& get_log()
{
	return mLog;
}

const std::string & get_log_string()
{
	return mLog_string;
}

std::string message::to_string() const
{
	std::string retval;

	switch (type)
	{
	case level::error:   retval += "Error   : "; break;
	case level::info:    retval += "Info    : "; break;
	case level::warning: retval += "Warning : "; break;
	case level::debug:   retval += "Debug   : "; break;
	}

	if (is_file)
	{
		retval += engine::fs::path(file).relative_path().string();
		if (column >= 0)
		{
			retval += "(" + std::to_string(column);
			if (row >= 0)
				retval += ", " + std::to_string(row);
			retval += ")";
		}
		retval += " ";
	}
	retval += msg;
	return retval;
}

void message::set_to_current_time()
{
	std::time_t time = std::time(nullptr);
	std::tm timeinfo;

#ifdef __linux__
	localtime_r(&time, &timeinfo);
#else
	localtime_s(&timeinfo, &time);
#endif

	char time_str[100];
	std::size_t time_str_length = strftime(time_str, 100, "%T", &timeinfo);
	time_stamp = std::string(time_str, time_str_length);
}

}