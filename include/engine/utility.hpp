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
#include <typeinfo>

namespace util
{

class any
{
public:
	any()
	{
	}

	~any()
	{
		clear();
	}

	any(any& pA)
	{
	}

	template<class T>
	void set()
	{
		typedef std::decay<T>::type type;
		mData.reset(dynamic_cast<placeholder*>(new holder<type>()));
	}

	template<class T>
	void set(const T& pData)
	{
		typedef std::decay<T>::type type;
		mData.reset(dynamic_cast<placeholder*>(new holder<type>(pData)));
	}

	template<class T>
	any(const T& pData)
	{
		set(pData);
	}

	template<class T>
	any& operator=(const T& pData)
	{
		set(pData);
		return *this;
	}

	bool empty() const
	{
		return !mData;
	}

	template<class T>
	bool check() const
	{
		typedef std::decay<T>::type type;
		if (empty())
			return false;
		return typeid(type) == mData->get_type_info();
	}

	template<class T>
	T& get() const
	{
		typedef std::decay<T>::type type;
		assert(check<T>());
		return dynamic_cast<holder<type>*>(mData.get())->get();
	}

	void clear()
	{
		mData.reset();
	}

private:
	class placeholder
	{
	public:
		virtual ~placeholder() {}
		virtual const std::type_info& get_type_info() const = 0;
		virtual placeholder* clone() const = 0;
	};

	template<class T>
	class holder : public placeholder
	{
	public:
		holder() {}
		holder(const T& pData)
			: mData(pData)
		{}
		~holder() { }

		virtual const std::type_info& get_type_info() const
		{
			return typeid(T);
		}

		virtual placeholder* clone() const
		{
			return new holder<T>(mData);
		}

		T& get()
		{
			return mData;
		}
	private:
		T mData;
	};

	std::unique_ptr<placeholder> mData;
};

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

template<typename T>
class optional
{
public:
	optional(){}

	optional(const optional& pOptional)
	{
		if (pOptional.has_value())
			mData = T(*pOptional);
	}

	optional(const T& pValue) :
		mData(pValue)
	{
	}

	template<typename...T_ARGS>
	optional(in_place_t pIn_place, T_ARGS&&... pArgs) :
		mData(std::forward<T_ARGS>(pArgs)...)
	{
	}

	T& operator*()&
	{
		assert(mHas_value);
		return mData;
	}

	const T& operator*() const&
	{
		assert(mHas_value);
		return mData;
	}

	T& operator->()
	{
		assert(mHas_value);
		return mData;
	}

	const T& operator->() const
	{
		assert(mHas_value);
		return mData;
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
	T mData;
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
/// Should be checked with tracking_ptr::is_enabled beforehand
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
inline T floor_align(T pVal, T pAlign)
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
inline std::string safe_string(const char* str)
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
	if (str.empty())
		return 0;
	return static_cast<char>(std::stoi(str, i));
}

template<>
inline int to_numeral<int>(const std::string& str, size_t *i)
{
	if (str.empty())
		return 0;
	return std::stoi(str, i);
}

template<>
inline float to_numeral<float>(const std::string& str, size_t *i)
{
	if (str.empty())
		return 0;
	return std::stof(str, i);
}

template<>
inline double to_numeral<double>(const std::string& str, size_t *i)
{
	if (str.empty())
		return 0;
	return std::stod(str, i);
}


template<typename T>
T to_numeral(const std::string& str, std::string::iterator& iter)
{
	if (str.empty())
		return 0;
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
inline T clamp(T v, T min, T max)
{
	if (v < min) return min;
	if (v > max) return max;
	return v;
}


// Pingpong array
template<typename T>
inline T pingpong_index(T v, T end)
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
