#include <engine/filesystem.hpp>
#include <engine/log.hpp>
#include <fstream>
#include <iostream>
#include <cassert>

namespace logger {

static std::string mLog;
static std::ofstream mLog_file;
static size_t mSub_routine_level;

void initialize(const std::string & pOutput)
{
	if (mLog_file)
		mLog_file.close();

	mLog_file.open(pOutput.c_str());
	if (!mLog_file)
		std::cout << "Failed to open log output file\n";
	else
		std::cout << "Log file at '" + pOutput + "'\n";

	mSub_routine_level = 0;
}

void print(level pType, const std::string& pMessage)
{
	std::string type;
	switch (pType)
	{
	case level::error:   type = "ERROR  ";   break;
	case level::info:    type = "INFO   ";    break;
	case level::warning: type = "WARNING"; break;
	case level::debug:   type = "DEBUG  ";   break;
	}

	std::string message = type + " : ";
	for (size_t i = 0; i < mSub_routine_level; i++)
		message += "| ";
	message += pMessage + "\n";

	mLog += message;

	if (mLog_file)
	{
		mLog_file << message; // Save to file
		mLog_file.flush();
	}

	// Printing to the console is disabled to
	// to remove the overhead and the redundancy.
#ifndef LOCKED_RELEASE_MODE
	std::cout << message; // Print to console  
#endif
}

void print(const std::string& pFile, int pLine, int pCol, level pType, const std::string& pMessage)
{
	std::string message = pFile;
	message += " ( ";
	message += std::to_string(pLine);
	message += ", ";
	message += std::to_string(pCol);
	message += " ) : ";
	message += pMessage;

	print(pType, message);
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

const std::string & get_log()
{
	return mLog;
}

void start_sub_routine()
{
	++mSub_routine_level;
}

void end_sub_routine()
{
	assert(mSub_routine_level > 0);
	--mSub_routine_level;
}


sub_routine::sub_routine()
{
	start_sub_routine();
}

sub_routine::~sub_routine()
{
	end_sub_routine();
}

void sub_routine::end()
{
	end_sub_routine();
}

}