#ifndef UTILITY_UTILITY_HPP
#define UTILITY_UTILITY_HPP

#include <string>
#include <iostream>
#include <memory>
#include <list>
#include <cassert>
#include <type_traits>

namespace util
{

template<typename T>
static T floor_align(T v, T scale)
{
	return std::floor(v / scale)*scale;
}

class nocopy {
public:
	nocopy(){}
private:
	nocopy(const nocopy&) = delete;
	nocopy& operator=(const nocopy&) = delete;
};

static std::string safe_string(const char* str)
{
	if (str == nullptr)
		return std::string(); // Empty string
	return str;
}


template<typename T>
static T to_numeral(const std::string& str, size_t *i = nullptr)
{
	static_assert(std::is_arithmetic<T>::value, "Requires arithmetic type");
	return 0;
}

template<>
static char to_numeral<char>(const std::string& str, size_t *i)
{
	return (char)std::stoi(str, i);
}

template<>
static int to_numeral<int>(const std::string& str, size_t *i)
{
	return std::stoi(str, i);
}

template<>
static float to_numeral<float>(const std::string& str, size_t *i)
{
	return std::stof(str, i);
}

template<>
static double to_numeral<double>(const std::string& str, size_t *i)
{
	return std::stod(str, i);
}


template<typename T>
static T to_numeral(const std::string& str, std::string::iterator& iter)
{
	size_t i = 0;
	T val = to_numeral<T>(std::string(iter, str.end()), &i);
	iter += i;
	return val;
}

template<typename T>
static T to_numeral(const std::string& str, std::string::const_iterator& iter)
{
	size_t i = 0;
	T val = to_numeral<T>(std::string(iter, str.end()), &i);
	iter += i;
	return val;
}

class named
{
	std::string name;
public:
	void set_name(const std::string& str)
	{
		name = str;
	}
	const std::string& get_name()
	{
		return name;
	}
};

template<typename T>
static inline T clamp(T v, T min, T max)
{
	if (v < min) return min;
	if (v > max) return max;
	return v;
}


// Pingpong array
template<typename T>
T pingpong_index(T v, T end)
{
	assert(end != 0);
	return ((v / end) % 2) ? end - (v%end) : (v%end);
}

// Gives error handling in the form a return.
// Simply provide an error message and it will print the message
// if the error is not handled by handle_error().
// Ex: return "Failed to do thing!";
// Error codes can be provided as well.
// To specify no error, simply return error::NOERROR or return 0.
// The main purpose was to rid of all the std::cout and make
// error handling simply one liners.
class error
{
	struct handler{
		bool unhandled; 
		std::string message;
		int code;
	};
	std::shared_ptr<handler> err;
public:

	static const int NOERROR = 0, ERROR = 1;

	error()
	{
		err.reset(new handler);
		err->code = NOERROR;
		err->unhandled = false;
	}
	error(const error& A)
	{
		err = A.err;
	}
	error(const std::string& message)
	{
		err.reset(new handler);
		err->unhandled = true;
		err->message = message;
		err->code = ERROR;
	}
	error(int code)
	{
		err.reset(new handler);
		err->unhandled = (code != NOERROR);
		err->code = code;
	}

	~error()
	{
		if (err->unhandled && err.unique())
		{
			if (err->message.empty())
				std::cout << "Error Code :" << err->code << "\n";
			else
				std::cout << "Error : " << err->message << "\n";
		}
	}

	error& handle_error()
	{
		err->unhandled = false;
		return *this;
	}

	const std::string& get_message()
	{
	    return err->message;
	}
	
	int get_error_code()
	{
		return err->code;
	}

	error& operator=(const error& A)
	{
		err = A.err;
		return *this;
	}
	
	bool has_error()
	{
		return err->code != NOERROR;
	}

	bool is_handled()
	{
		return !err->unhandled;
	}

	explicit operator bool()
	{
		return err->code != NOERROR;
	}
};


}

#endif