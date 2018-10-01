#pragma once

#include <ostream>
#include <cassert>
#include <vector>
#include <any>
#include <ctime>

namespace wge::log
{

// Stream used by this log for message building operations
using log_ostream = std::ostream;

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
};

using userdata_t = std::any;

namespace detail
{
// Stores the userdata until it is consumed by a
// stream.
struct userdata_carrier
{
	userdata_t userdata;
};
log_ostream& operator<<(log_ostream&, userdata_carrier&);

} // namespace detail

// Adds "something" as userdata in your message.
// This is primarily meant for a custom log UI that
// can display extra information about a message eg
// stack info in a script.
// Usage: log::out << log::userdata(234) << "my message" << log::endm;
detail::userdata_carrier userdata(userdata_t pAny);

struct message
{
	std::string string;
	level severity_level{ level::unknown };
	line_info line_info;
	std::time_t time_stamp;

	userdata_t userdata;

	std::string to_string(bool pAnsi_color = false) const;
};

using log_container = std::vector<message>;
const log_container& get_log();

// Returns true if the file was successfully opened
bool open_file(const char* pFile);

// Prints an assertion message as a warning. Returns the boolean result of the expression.
bool soft_assert(bool pExpression, const char* pMessage, line_info);

// Get output stream for constructing a messsage. Remember to call flush() or endm when complete!
extern log_ostream& out;
// Outputs the entire stream in a single message and then clears the stream
// for a new message.
void flush();
// Similar to std::endl, instead this just calls flush()
log_ostream& endm(log_ostream& os);
log_ostream& operator<<(log_ostream&, line_info);
log_ostream& operator<<(log_ostream&, level);

// Sets the severity and returns the out stream
log_ostream& info();
log_ostream& debug();
log_ostream& warning();
log_ostream& error();

} // namespace wge::log

// Generates a line_info struct containing the current line and file.
// Usage: log::out << WGE_LI << "My message";
#define WGE_LI wge::log::line_info{ __LINE__, -1, __FILE__, true }
// Strict assert macro.
#define WGE_ASSERT(A) assert(A)

#define WGE_SASSERT_MSG(Expression, Message) wge::log::soft_assert(Expression, #Expression " - " Message, WGE_LI);
#define WGE_SASSERT(Expression) wge::log::soft_assert(Expression, #Expression, WGE_LI);