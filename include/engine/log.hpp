#ifndef ENGINE_LOG_HPP
#define ENGINE_LOG_HPP

#include <string>

namespace logger {

enum class level
{
	error,
	info,
	warning,
	debug
};

void initialize(const std::string& pOutput);

void print(level pType, const std::string& pMessage);

void print(const std::string& pFile, int pLine, int pCol, level pType, const std::string& pMessage);

void error(const std::string& pMessage);

void warning(const std::string& pMessage);

void info(const std::string& pMessage);

class sub_routine
{
public:
	sub_routine();
	~sub_routine();
	void end();
};

void start_sub_routine();
void end_sub_routine();

}

#endif