#ifndef ENGINE_LOG_HPP
#define ENGINE_LOG_HPP

#include <string>
#include <vector>

namespace logger {

enum class level
{
	error,
	info,
	warning,
	debug
};

struct message
{
public:
	bool is_file;
	std::string file, msg, time_stamp;
	level type;
	int column, row; // <0 will mean there is no column or row

	std::string to_string() const;
	void set_to_current_time();
};

void initialize(const std::string& pOutput);

message print(const message& pMessage);
message print(level pType, const std::string& pMessage);
message print(const std::string& pFile, int pLine, level pType, const std::string& pMessage);
message print(const std::string& pFile, int pLine, int pCol, level pType, const std::string& pMessage);

void error(const std::string& pMessage);

void warning(const std::string& pMessage);

void info(const std::string& pMessage);

const std::vector<message>& get_log();
const std::string& get_log_string();

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