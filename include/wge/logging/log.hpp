#pragma once

#include <ostream>
#include <cassert>
#include <vector>
#include <any>
#include <ctime>
#include <string_view>
#include <sstream>

namespace wge::log
{

// Type used to store userdata
using userdata_t = std::any;

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

struct message
{
	std::string string;
	level severity_level{ level::unknown };
	line_info line_info;
	std::time_t time_stamp{ 0 };

	userdata_t userdata;

	std::string to_string(bool pAnsi_color = false) const;
};

// Adds "something" as userdata in your message.
// This is primarily meant for a custom log UI that
// can display extra information about a message eg
// stack info in a script.
// Usage: log::out << log::userdata(234) << "my message" << log::endm;
struct userdata
{
	template <typename T>
	userdata(T&& pAny) :
		data(std::forward<T>(pAny))
	{}
	userdata_t data;
};

class message_builder
{
public:
	template <typename T>
	message_builder& operator<<(T&& pIn)
	{
		using type = std::decay_t<T>;
		if constexpr (std::is_same_v<type, userdata>)
			mMessage.userdata = std::move(pUserdata.data);
		else if constexpr (std::is_same_v<type, level>)
			mMessage.severity_level = pIn;
		else if constexpr (std::is_same_v<type, line_info>)
			mMessage.line_info = pIn;
		else
			mStream << pIn;
		return *this;
	}

	// Return the final message and prepare for a new message
	message finalize();

private:
	std::ostringstream mStream;
	log::message mMessage;
};

using log_container = std::vector<message>;
const log_container& get_log();

// Returns true if the file was successfully opened
bool open_file(const char* pFile);

// Prints an assertion message as a warning. Returns the boolean result of the expression.
bool soft_assert(bool pExpression, const char* pMessage, line_info);

// Get output stream for constructing a messsage. Remember to call flush() or endm when complete!
extern message_builder& out;
// Outputs the entire stream in a single message and then clears the stream
// for a new message.
void flush();
// Similar to std::endl, instead this just calls flush()
std::ostream& endm(std::ostream& os);

// Sets the severity and returns the out stream
message_builder& info();
message_builder& debug();
message_builder& warning();
message_builder& error();

} // namespace wge::log

// Generates a line_info struct containing the current line and file.
// Usage: log::out << WGE_LI << "My message";
#define WGE_LI wge::log::line_info{ __LINE__, -1, __FILE__, true }
// Strict assert macro.
#define WGE_ASSERT(A) assert(A)

#define WGE_SASSERT_MSG(Expression, Message) wge::log::soft_assert(Expression, #Expression " - " Message, WGE_LI);
#define WGE_SASSERT(Expression) wge::log::soft_assert(Expression, #Expression, WGE_LI);