#pragma once

#include <type_traits>
#include <memory>

namespace wge::util
{

// Obtain raw pointer from pointer-like objects (shared_ptr, unique_ptr...)
template <typename T>
inline constexpr auto to_address(T&& pPtr)
{
	return &(*pPtr);
}

// A unique_ptr wrapper that allows the underlaying polymorphic object
// to be copied. Think of it as a strongly typed std::any where only the base
// or its sub-classes can be stored.
template <typename T>
class copyable_ptr
{
	friend class copyable_ptr;
public:
	template <typename Tcopier>
	copyable_ptr(std::unique_ptr<T> pPtr, Tcopier&& pCopier) noexcept :
		mPtr(std::move(pPtr)),
		mCopier(std::move(pCopier))
	{}
	copyable_ptr(copyable_ptr&&) = default;
	copyable_ptr(const copyable_ptr& pOther) :
		mPtr(pOther.mCopier(pOther.mPtr)),
		mCopier(pOther.mCopier)
	{}
	copyable_ptr(std::nullptr_t) noexcept
	{}
	copyable_ptr() noexcept
	{}
	
	T* get() const noexcept
	{
		return mPtr.get();
	}

	T* release()
	{
		return mPtr.release();
	}

	void reset()
	{
		mPtr.reset();
	}

	bool valid() const noexcept
	{
		return mPtr != nullptr;
	}

	operator bool() const noexcept
	{
		return valid();
	}

	T* operator->() const noexcept
	{
		return mPtr.get();
	}

	T& operator*() const noexcept
	{
		return *mPtr;
	}

private:
	std::function<std::unique_ptr<T>(const std::unique_ptr<T>&)> mCopier;
	std::unique_ptr<T> mPtr;
};

namespace detail
{

template <typename T, typename Tbase>
constexpr auto make_copier() noexcept
{
	return [](const std::unique_ptr<Tbase>& pOrig) -> std::unique_ptr<Tbase>
	{
		return std::make_unique<T>(static_cast<T&>(*pOrig));
	};
}

} // namespace detail

template <typename T, typename Tbase = T, typename...Targs>
copyable_ptr<Tbase> make_copyable_ptr(Targs&&...pArgs)
{
	return{ std::make_unique<T>(std::forward<Targs>(pArgs)...), detail::make_copier<T, Tbase>() };
}

} // namespace wge::util
