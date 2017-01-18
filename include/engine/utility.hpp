#ifndef UTILITY_UTILITY_HPP
#define UTILITY_UTILITY_HPP

#include <string>
#include <iostream>
#include <memory>
#include <list>
#include <cassert>
#include <type_traits>
#include <cmath>
#include <fstream>

namespace util
{
/// Similar to optional but optimized for a pointer
template<typename T>
class optional_pointer
{
public:
	optional_pointer()
	{
		mPointer = nullptr;
	}

	optional_pointer(const optional_pointer& pA)
	{
		mPointer = pA.mPointer;
	}

	optional_pointer(T* pPointer)
	{
		mPointer = pPointer;
	}

	optional_pointer& operator=(const optional_pointer& pR)
	{
		mPointer = pR.mPointer;
		return *this;
	}

	optional_pointer& operator=(T* pPointer)
	{
		mPointer = pPointer;
		return *this;
	}

	bool has_value() const
	{
		return mPointer != nullptr;
	}

	operator bool() const
	{
		return has_value();
	}

	operator T*() const
	{
		assert(has_value());
		return mPointer;
	}

	T& operator*() const
	{
		assert(has_value());
		return *mPointer;
	}

	T* operator->() const
	{
		assert(has_value());
		return mPointer;
	}

	void reset()
	{
		mPointer = nullptr;
	}

private:
	T* mPointer;
};

class in_place_t {};
static const in_place_t in_place;

/// Similar to boost/std's optional object.
/// Prevents access when there is no 
/// contained object. This does not construct
/// the object if there is none and doesn't
/// dynamically allocate memory.
template<typename T>
class optional
{
public:
	optional()
	{
		mHas_value = false;
	}

	~optional()
	{
		if (mHas_value &&
			std::is_destructible<T>::value)
		{
			get_data()->~T();
		}
	}

	optional(const optional& pOptional)
	{
		mHas_value = pOptional.mHas_value;
		if (mHas_value)
			new (get_data()) T(*reinterpret_cast<const T*>(pOptional.mData));
	}

	optional(const T& pValue)
	{
		static_assert(std::is_copy_constructible<T>::value, "T needs to be copy constructable");
		new(get_data()) T(pValue);
		mHas_value = true;
	}

	template<typename...T_ARGS>
	optional(in_place_t pIn_place, T_ARGS&&... pArgs)
	{
		static_assert(std::is_copy_constructible<T>::value, "T needs to be copy constructable");
		new(get_data()) T(std::forward<T_ARGS>(pArgs)...);
		mHas_value = true;
	}

	T& operator*()&
	{
		assert(mHas_value);
		return *get_data();
	}

	const T& operator*() const&
	{
		assert(mHas_value);
		return *get_data();
	}

	T& operator->()
	{
		assert(mHas_value);
		return *get_data();
	}

	const T& operator->() const
	{
		assert(mHas_value);
		return *get_data();
	}

	bool has_value() const
	{
		return mHas_value;
	}

	operator bool() const
	{
		return mHas_value;
	}
private:

	T* get_data()
	{
		return reinterpret_cast<T*>(&mData);
	}

	// Allocates space for the object WITHOUT any construction
	char mData[sizeof(T)];

	bool mHas_value;
};

/// Notifies all references (tracking_ptr) that it has
/// been destroyed.
class tracked_owner
{
public:
	tracked_owner()
	{
		mIs_valid.reset(new bool);
		*mIs_valid = true;
	}

	~tracked_owner()
	{
		*mIs_valid = false;
	}

	template<typename T>
	friend class tracking_ptr;

private:
	std::shared_ptr<bool> mIs_valid;
};

/// References an object of tracked_owner.
/// tracked_owner notifies this object whenever
/// it has been destroyed. Also prevents unwanted
/// access when referenced object is destroyed.
/// Should be checked with tracking_ptr::is_valid beforehand
template<typename T>
class tracking_ptr
{
public:
	tracking_ptr()
	{
		static_assert(std::is_convertible<T, tracked_owner>::value, "T is not tracked_owner");
	}
	~tracking_ptr() {}

	tracking_ptr(T& a)
	{
		set(a);
	}

	tracking_ptr(const tracking_ptr& a)
	{
		set(a);
	}

	tracking_ptr& operator=(const tracking_ptr& r)
	{
		set(r);
		return *this;
	}

	tracking_ptr& operator=(T& r)
	{
		set(r);
		return *this;
	}

	void set(T& r)
	{
		assert(r.mIs_valid != nullptr);
		mIs_valid = r.mIs_valid;
		mPointer = &r;
	}

	void set(const tracking_ptr& r)
	{
		if (!r.is_valid())
			return;
		mIs_valid = r.mIs_valid;
		mPointer = r.mPointer;
	}

	void reset()
	{
		mIs_valid.reset();
	}

	bool is_valid() const
	{
		return mIs_valid != nullptr && *mIs_valid != false;
	}

	T* get() const
	{
		assert(mIs_valid != nullptr);
		assert(*mIs_valid != false);
		return mPointer;
	}

	T& operator*() const
	{
		return *get();
	}

	T* operator->() const
	{
		return get();
	}

	operator bool() const
	{
		return is_valid();
	}

private:
	std::shared_ptr<bool> mIs_valid;
	T* mPointer;
};

/// Floor a value to a specific alignment.
/// Ex. 
template<typename T>
static T floor_align(T pVal, T pAlign)
{
	return std::floor(pVal / pAlign)*pAlign;
}

/// Prevents unwanted copying of a class
class nocopy {
public:
	nocopy(){}
private:
	nocopy(const nocopy&) = delete;
	nocopy& operator=(const nocopy&) = delete;
};

/// std::string doesn't like null pointers so this
/// just returns an empty std::string when there is one.
static std::string safe_string(const char* str)
{
	if (str == nullptr)
		return std::string();
	return str;
}

/// Templated version of the sto* functions
template<typename T>
inline T to_numeral(const std::string& str, size_t *i = nullptr)
{
	static_assert(std::is_arithmetic<T>::value, "Requires arithmetic type");
	return 0;
}

template<>
inline char to_numeral<char>(const std::string& str, size_t *i)
{
	return static_cast<char>(std::stoi(str, i));
}

template<>
inline int to_numeral<int>(const std::string& str, size_t *i)
{
	return std::stoi(str, i);
}

template<>
inline float to_numeral<float>(const std::string& str, size_t *i)
{
	return std::stof(str, i);
}

template<>
inline double to_numeral<double>(const std::string& str, size_t *i)
{
	return std::stod(str, i);
}


template<typename T>
T to_numeral(const std::string& str, std::string::iterator& iter)
{
	size_t i = 0;
	T val = to_numeral<T>(std::string(iter, str.end()), &i);
	iter += i;
	return val;
}

template<typename T>
T to_numeral(const std::string& str, std::string::const_iterator& iter)
{
	size_t i = 0;
	T val = to_numeral<T>(std::string(iter, str.end()), &i);
	iter += i;
	return val;
}

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

enum class log_level
{
	error,
	info,
	warning,
	debug
};

void log_print(log_level pType, const std::string& pMessage);

void log_print(const std::string& pFile, int pLine, int pCol, log_level pType, const std::string& pMessage);

void error(const std::string& pMessage);

void warning(const std::string& pMessage);

void info(const std::string& pMessage);

}

#endif
