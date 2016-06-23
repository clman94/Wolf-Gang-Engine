#ifndef UTILITY_UTILITY_HPP
#define UTILITY_UTILITY_HPP

#include <string>
#include <iostream>
#include <memory>


namespace utility
{

// Allows one class to dominate and hides a hidden item that
// can be retrieved for shady stuff.
template<typename T1, typename T2_S>
class shadow_pair
	: public T1
{
	T2_S shadow;
public:
	template<class T1, typename T2_S>
	friend T2_S& get_shadow(shadow_pair<T1, T2_S>& A);
};


// Get reference of shadow in the shadow pair.
template<class T1, typename T2_S>
static T2_S& get_shadow(shadow_pair<T1, T2_S>& A)
{
	return A.shadow;
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

	// Specify not to print message.
	// Returns whether or not it had an error.
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

	explicit operator bool()
	{
		return err->code != NOERROR;
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
	
	ret& operator=(const ret& A)
	{
		item = A.item;
		err = A.err;
		has_return = A.has_return;
		return *this;
	}

	ret(const ret& A)
	{
		*this = A;
	}

	T& get_return()
	{
		return item;
	}
	
	error& get_error()
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
static T& get_return(ret<T> r)
{
	return r.get_return();
}

}

#endif