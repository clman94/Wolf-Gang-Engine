#ifndef ENGINE_LOG_HPP
#define ENGINE_LOG_HPP

#include <string>
#include <vector>

namespace logger {

enum class level
{
	info,
	debug,
	warning,
	error,
};

// Extra data to include with message
class message_ext
{
public:
	virtual ~message_ext() {}
};

struct message
{
public:
	message() : is_file(false), column(-1), row(-1){}
	message(level pType) : is_file(false), column(-1), row(-1), type(pType){}

	bool is_file;
	std::string file, msg, time_stamp;
	level type;
	int column, row; // <0 will mean there is no column or row

	std::shared_ptr<message_ext> ext;

	std::string to_string() const;
	void set_to_current_time();
};

// Sets the file in which to store the log.
void initialize(const std::string& pOutput);

message print(const message& pMessage);
message print(level pType, const std::string& pMessage);
message print(const std::string& pFile, int pLine, level pType, const std::string& pMessage);
message print(const std::string& pFile, int pLine, int pCol, level pType, const std::string& pMessage);

void error(const std::string& pMessage);   // Shortcut for print(level::XXX, pMessage);
void warning(const std::string& pMessage); // "
void info(const std::string& pMessage);    // "
void debug(const std::string& pMessage);   // "

const std::vector<message>& get_log();
const std::string& get_log_string();

}

#endif