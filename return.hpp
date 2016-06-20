#ifndef UTILITY_RETURN_HPP
#define UTILITY_RETURN_HPP

#include <string>
#include <iostream>
#include <memory>


namespace utility
{

// Gives error handling in the form a return.
// Simply provide an error message and it will print the message
// if the error is not handled by handle_error().
// Ex: return "Failed to do thing!";
// Error codes can be provided as well.
// To specify no error, simply return 0.
// The main purpose was to rid of all the std::cout and make
// error handling simply one liners.
class error
{
	std::shared_ptr<bool> err;
	std::string error_str;
	int error_code;
public:
	error()
	{
		err.reset(new bool(false));
		error_code = 0;
	}
	error(const error& A)
	{
		err = A.err;
		error_str = A.error_str;
		error_code = A.error_code;
	}
	error(const std::string& message)
	{
		err.reset(new bool(true));
		error_str = message;
	}
	error(int code)
	{
		error_code = code;
		err.reset(new bool(code != 0));
	}

	~error()
	{
		if (*err && err.unique())
			std::cout << "Error : " << error_str << " : \n";
	}

	// Specify not to print message.
	// Returns whether or not it had an error.
	bool handle_error()
	{
		bool prev = *err;
		*err = false;
		return prev;
	}

	const std::string& get_message()
	{
	    return error_str;
	}
	
	int get_error_code()
	{
		return error_code;
	}

	error& operator=(const error& A)
	{
		err = A.err;
		error_str = A.error_str;
		error_code = A.error_code;
		return *this;
	}
	
	bool has_error()
	{
	    return *err;
	}

	explicit operator bool()
	{
		return *err;
	}
};

// Allows you to return an item or return an error.
// Takes normal return values as "return value;".
// To return an error, simply:
// return utility::error("Failed to do thing!");
template<typename T>
class ret
{
	T item;
	error err;
	int has_return;
public:

	ret()
	{
		has_return = false;
	}
	
    template<typename T1>
	ret(const T1& A)
	{
		item = A;
		has_return = true;
	}
	
	ret(const error& A)
	{
		err = A;
		has_return = false;
	}
	
	T& operator=(const ret& A)
	{
		return A.item;
	}

	T& get()
	{
		return item;
	}
	
	const error& get_error()
	{
	    return err;
	}
	
	// Check for no error
	explicit operator bool()
	{
		return !err.has_error() && has_return;
	}
};

template<typename T>
static T& get_ret(ret<T> r)
{
	return r.get();
}

}

#endif