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

/*
class light_string
{
	char* str;
public:
	light_string()
	{
		str = nullptr;
	}
	~light_string()
	{
		clean();
	}

	void clean()
	{
		if (str)
			delete str;
	}

	light_string& operator=(const char* s)
	{
		if (s == nullptr)
		{
			clean();
			return *this;
		}
		size_t len = std::strlen(s);
		str = new char[len + 1];
		std::strcpy(str, s);
		return *this;
	}

	size_t get_length()
	{
		if (str == nullptr) return 0;
		return std::strlen(str);
	}

	explicit operator bool()
	{
		return str != nullptr;
	}
};*/

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

// Stores two vectors that will always be the same size.
// Provides an interface "somewhat" similar to a vector.
template<typename T1, typename T2>
class vector_pair
{
public:
	typedef std::vector<T1> first_vector;
	typedef std::vector<T2> second_vector;
	typedef std::pair<T1*, T2*> pair_ptr;

	void resize(size_t a)
	{
		vec1.resize(a);
		vec2.resize(a);
		assert(vec1.size() == vec2.size());
	}

	void reserve(size_t a)
	{
		vec1.reserve(a);
		vec2.reserve(a);
	}

	pair_ptr at(size_t i)
	{
		assert(size() > 0);
		assert(i < size());

		T1 *t1 = &vec1[i];
		T2 *t2 = &vec2[i];
		return{ t1, t2 };
	}

	pair_ptr operator[](size_t i)
	{
		return at(i);
	}

	size_t size()
	{
		assert(vec1.size() == vec2.size());
		return vec1.size();
	}

	template<class... TV1, class... TV2>
	pair_ptr emplace_back(TV1&&... V1, TV2&&... V2)
	{
		vec1.emplace_back(std::forward<TV1>(V1)...);
		vec2.emplace_back(std::forward<TV2>(V2)...);
		assert(vec1.size() == vec2.size());
		return at(vec1.size() - 1);
	}

	pair_ptr emplace_back()
	{
		vec1.emplace_back();
		vec2.emplace_back();
		assert(vec1.size() == vec2.size());
		return at(vec1.size() - 1);
	}

	template<class TV1, class TV2>
	pair_ptr push_back(TV1&& V1, TV2&& V2)
	{
		vec1.push_back(std::forward<TV1>(V1));
		vec2.push_back(std::forward<TV2>(V2));
		assert(vec1.size() == vec2.size());
		return at(vec1.size() - 1);
	}

	pair_ptr push_back()
	{
		vec1.push_back();
		vec2.push_back();
		assert(vec1.size() == vec2.size());
		return at(vec1.size() - 1);
	}

	void erase(size_t pos)
	{
		vec1.erase(vec1.begin() + pos);
		vec2.erase(vec2.begin() + pos);
		assert(vec1.size() == vec2.size());
	}

	/*void erase(first_vector::iterator &pos)
	{
		vec1.erase(pos);
		vec2.erase(pos);
		assert(vec1.size() == vec2.size());
	}

	void erase(second_vector::iterator &pos)
	{
		vec1.erase(pos);
		vec2.erase(pos);
		assert(vec1.size() == vec2.size());
	}

	first_vector::iterator& first_begin()
	{
		return vec1.begin();
	}

	first_vector::iterator& first_end()
	{
		return vec1.end();
	}

	second_vector::iterator& second_begin()
	{
		return vec2.begin();
	}

	second_vector::iterator& second_end()
	{
		return vec2.end();
	}*/

	const first_vector& first()
	{
		return vec1;
	}

	const second_vector& second()
	{
		return vec2;
	}

private:
	first_vector  vec1;
	second_vector vec2;
};

template<typename T>
static inline T clamp(T v, T min, T max)
{
	if (v < min) return min;
	if (v > max) return max;
	return v;
}

// Allows one class to dominate and hides a hidden item that
// can be retrieved for shady stuff.
// Why have such a strangely shady class? Convienence, of course!
template<class T1, typename T2_S>
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

// Pingpong array
template<typename T>
T pingpong_index(T v, T end)
{
	assert(end != 0);
	return ((v / end) % 2) ? end - (v%end) : (v%end);
}

// Iterate through sequences with features like pingponging and looping
template<typename T>
class seq_tracker
{
	T counter;
	T proc;
	T start, end;
	int type;

	void calculate_counter()
	{
		if (type == LINEAR_LOOP)
			proc = (counter % (end - start)) + start;
		if (type == LINEAR_CLAMP)
			proc = clamp(counter, start, end - 1);
		if (type == PING_PONG)
		{
			proc = counter%end;
			if ((counter / end) % 2)
				proc = end - proc;
		}
	}

public:

	enum count_type
	{
		LINEAR_LOOP,
		LINEAR_CLAMP,
		PING_PONG
	};

	seq_tracker()
	{
		counter = 0;
		proc = 0;
		type = LINEAR_LOOP;
	}

	operator T()
	{
		return proc;
	}

	void set_count(T n)
	{
		counter = n;
		calculate_counter();
	}

	T get_count()
	{
		return proc;
	}

	void set_type(int a)
	{
		type = a;
		calculate_counter();
	}

	void set_start(T n)
	{
		start = n;
	}

	void set_end(T n)
	{
		end = n;
	}

	// Only useful with type=LINEAR_CLAMP
	bool is_finished()
	{
		return proc == end;
	}

	T step(T amount)
	{
		counter += amount;
		calculate_counter();
		return proc;
	}
};

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