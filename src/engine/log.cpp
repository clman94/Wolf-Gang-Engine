#include <engine/utility.hpp>
#include <engine/filesystem.hpp>

void util::log_print(log_level pType, const std::string& pMessage)
{
#ifndef LOCKED_RELEASE_MODE
	static std::ofstream mLog_file;

	if (!mLog_file.is_open()) // Open the log file when needed
		mLog_file.open("./data/log.txt");

	std::string type;
	switch (pType)
	{
	case log_level::error:   type = "ERROR";   break;
	case log_level::info:    type = "INFO";    break;
	case log_level::warning: type = "WARNING"; break;
	case log_level::debug:   type = "DEBUG";   break;
	}
	std::string message = type;
	message += " : ";
	message += pMessage;
	message += "\n";

	if (mLog_file.is_open())
	{
		mLog_file << message; // Save to file
		mLog_file.flush();
	}

	std::cout << message; // Print to console  
#endif // !LOCKED_RELEASE_MODE

}

void util::log_print(const std::string& pFile, int pLine, int pCol, log_level pType, const std::string& pMessage)
{
#ifndef LOCKED_RELEASE_MODE
	std::string message = pFile;
	message += " ( ";
	message += std::to_string(pLine);
	message += ", ";
	message += std::to_string(pCol);
	message += " ) : ";
	message += pMessage;

	log_print(pType, message);
#endif // !LOCKED_RELEASE_MODE
}

void util::error(const std::string& pMessage)
{
	log_print(log_level::error, pMessage);
}

void util::warning(const std::string& pMessage)
{
	log_print(log_level::warning, pMessage);
}

void util::info(const std::string& pMessage)
{
	log_print(log_level::info, pMessage);
}