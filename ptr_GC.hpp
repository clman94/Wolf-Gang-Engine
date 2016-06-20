#ifndef PTR_GC_HPP
#define PTR_GC_HPP

#include <memory>
#include <iostream>
#include <atomic>

namespace engine
{

// A wrapper that manages a std::shared_ptr.
// It allows these pointers to have ownership or not.
// All owners are unified under an instance of shared_ptr.
// All non-owners are each seperate and do not delete pointer.

// Automatically takes ownership when given a pointer.
// Automatically prevents deletion if not a pointer.

template<typename T>
class ptr_GC
{
	std::shared_ptr<T> _ptr;
	bool _owner;

	template<typename T>
	struct D {
		D() : released(false){}

		void operator()(T* p)
		{
			if (!released)
			{
				printf("Deleting object\n");
				delete p;
			}
		}

		void release()
		{
			released = true;
		}
		bool released;
	};

public:
	ptr_GC()
	{
		_ptr = nullptr;
		_owner = false;
	}

	ptr_GC(T& obj, bool own = false)
	{
		reset();
		_ptr.reset(&obj, D<T>());
		_owner = own;
	}

	ptr_GC(T* obj, bool own = true)
	{
		reset();
		_ptr.reset(obj, D<T>());
		_owner = own;
	}

	template<typename T1>
	ptr_GC(const ptr_GC<T1>& obj)
	{
		reset();
		_owner = obj._owner;
		if (_owner)
			_ptr = obj._ptr;
		else
			_ptr.reset(obj._ptr.get(), D<T>());
	}

	~ptr_GC()
	{
		reset();
	}

	// Create object in heap
	void allocate_default()
	{
		if (_ptr)
			reset();
		_ptr.reset(new T);
		_owner = true;
	}

	bool is_null()
	{
		return _ptr == nullptr;
	}

	void reset()
	{
		if (!_owner && _ptr.unique())
		{
			auto deleter = std::get_deleter<D<T>>(_ptr);
			deleter->release();
		}
		_ptr.reset();
	}

	// Take ownership of object R
	template<typename T1>
	ptr_GC& operator=(T1* R)
	{
		reset();
		_ptr.reset(R, D<T>());
		_owner = true;
		return *this;
	}

	// Ownership of original object is preserved
	template<typename T1>
	ptr_GC& operator=(T1& R)
	{
		reset();
		_ptr.reset(&R, D<T>());
		_owner = false;
		return *this;
	}

	// Share ownership if allowed
	template<typename T1>
	ptr_GC& operator=(const ptr_GC<T1>& R)
	{
		reset();
		_owner = obj._owner;
		if (_owner)
			_ptr = obj._ptr;
		else
			_ptr.reset(obj._ptr.get(), D<T>());
		return *this;
	}

	// Get direct pointer
	T* get()
	{
		return _ptr.get();
	}

	ptr_GC<T> protect()
	{
		ptr_GC<T> temp;
		temp._ptr = _ptr;
		temp._owner = false;
		return temp;
	}

	bool is_owner()
	{
		return _owner;
	}

	int count()
	{
		return _ptr.use_count();
	}

	T* operator->()
	{
		return _ptr.get();
	}

	T& operator*()
	{
		return *_ptr.get();
	}

	template <typename>
	friend class ptr_GC;
};

template <typename T>
ptr_GC<T> allocate_ptr()
{
	ptr_GC<T> n;
	n.allocate_default();
	return n;
}

// Allocates on construction
template <typename T>
class ptr_GC_owner
	: public ptr_GC<T>
{
public:
	ptr_GC_owner()
	{
		allocate_default();
	}
};

}

#endif