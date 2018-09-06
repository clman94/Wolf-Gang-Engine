#pragma once

#include <type_traits>
#include <cassert>
#include <memory>

namespace wge
{
namespace util
{

class ref_counted
{
public:
	ref_counted()
	{
		mRefs = 1;
		mWeak_valid = std::make_shared<bool>(true);
	}

	virtual ~ref_counted()
	{
		*mWeak_valid = false;
	}

	void add_ref()
	{
		++mRefs;
	}

	void release_ref()
	{
		--mRefs;
		if (mRefs <= 0)
			destroy();
	}

	void destroy()
	{
		delete this;
	}

	std::shared_ptr<bool> get_weak_valid() const
	{
		return mWeak_valid;
	}

private:
	std::size_t mRefs;
	std::shared_ptr<bool> mWeak_valid;
};
template<class T, class Enable = void>
class ref
{};

template <class T>
class ref<T, typename std::enable_if<std::is_base_of<ref_counted, T>::value>::type>
{
public:
	// ref_counted objects are constructed with one reference
	// and when you pass it to a ref object, there are now 2 references.
	// When that ref object is destroyed, one reference is left and memory
	// is leaked. This method creates a ref_counted object and returns a ref
	// object with only ONE reference. You should use this over all else to
	// create ref_counted objects.
	template <class...Targs>
	static ref create(Targs&&... pArgs)
	{
		T* ptr = new T(std::forward<Targs>(pArgs)...);
		ref nref(ptr);
		ptr->release_ref(); // nref becomes the only owner of the reference
		return nref;
	}

	ref() :
		mPtr(nullptr)
	{}

	ref(T* pPtr)
	{
		mPtr = pPtr;
		add_ref();
	}

	ref(const ref& pRef)
	{
		mPtr = pRef.mPtr;
		add_ref();
	}

	~ref()
	{
		release_ref();
	}

	T* get() const
	{
		return mPtr;
	}

	ref& operator=(T* pPtr)
	{
		release_ref();
		mPtr = pPtr;
		add_ref();
		return *this;
	}

	ref& operator=(const ref& pRef)
	{
		release_ref();
		mPtr = pRef.mPtr;
		add_ref();
		return *this;
	}

	T* operator->() const
	{
		assert(mPtr);
		return mPtr;
	}

	T& operator*() const
	{
		assert(mPtr);
		return *mPtr;
	}

	bool operator==(const ref& pRef) const
	{
		return mPtr == pRef.mPtr;
	}

	bool operator!=(const ref& pRef) const
	{
		return mPtr != pRef.mPtr;
	}

	bool operator==(T* pPtr) const
	{
		return mPtr == pPtr;
	}

	bool operator!=(T* pPtr) const
	{
		return mPtr != pPtr;
	}

	operator bool() const
	{
		return mPtr != nullptr;
	}

	void reset()
	{
		release_ref();
		mPtr = nullptr;
	}

private:
	void add_ref()
	{
		if (mPtr)
			mPtr->add_ref();
	}

	void release_ref()
	{
		if (mPtr)
			mPtr->add_ref();
	}

private:
	T* mPtr;
};

template<class Tto, class Tfrom>
ref<Tto> cast_ref(ref<Tfrom> pRef)
{
	if (!pRef)
		return {};
	return{ dynamic_cast<Tto*>(pRef.get()) };
}

template <class T>
class weak_ref
{
public:
	weak_ref() :
		mPtr(nullptr)
	{}

	weak_ref(const weak_ref& pWeak)
	{
		mPtr = pWeak.mPtr;
		mWeak_valid = pWeak.mWeak_valid;
	}

	weak_ref(T* pRef)
	{
		mPtr = pRef;
		if (mPtr)
			mWeak_valid = mPtr->get_weak_valid();
	}

	weak_ref(const ref<T>& pRef)
	{
		mPtr = pRef.mPtr;
		if (mPtr)
			mWeak_valid = mPtr->get_weak_valid();
	}

	bool valid() const
	{
		return mPtr && mWeak_valid && *mWeak_valid;
	}

	bool expired() const
	{
		return !valid();
	}

	ref<T> lock() const
	{
		if (expired())
			return{};
		return{ mPtr };
	}

	bool operator==(const ref<T>& pRef)
	{
		return mPtr == pRef.mPtr;
	}

	bool operator!=(const ref<T>& pRef)
	{
		return mPtr != pRef.mPtr;
	}

	bool operator==(T* pPtr)
	{
		return mPtr == pPtr;
	}

	bool operator!=(T* pPtr)
	{
		return mPtr != pPtr;
	}

	weak_ref& operator=(T* pPtr)
	{
		mPtr = pPtr;
		if (mPtr)
			mWeak_valid = mPtr->get_weak_valid();
		return *this;
	}

	weak_ref& operator=(const ref<T>& pRef)
	{
		mPtr = pRef.mPtr;
		if (mPtr)
			mWeak_valid = mPtr->get_weak_valid();
		return *this;
	}

	weak_ref& operator=(const weak_ref& pRef)
	{
		mPtr = pRef.mPtr;
		mWeak_valid = pRef->mWeak_valid;
		return *this;
	}

	void reset()
	{
		mPtr = nullptr;
		mWeak_valid.reset();
	}

private:
	T* mPtr;
	std::shared_ptr<bool> mWeak_valid;
};

}
}