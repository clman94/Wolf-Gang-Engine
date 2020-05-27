#pragma once

#include <type_traits>
#include <memory>

namespace wge::util
{

template <typename Tto, typename Tfrom>
constexpr std::unique_ptr<Tto> dynamic_unique_cast(std::unique_ptr<Tfrom> pFrom) noexcept
{
	auto ptr = pFrom.release();
	if (auto cast = dynamic_cast<Tto*>(ptr))
	{
		return std::unique_ptr<Tto>(cast);
	}
	return nullptr;
}

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
	copyable_ptr() noexcept = default;
	copyable_ptr(std::nullptr_t) noexcept
	{}
	template <typename Tcopier>
	copyable_ptr(std::unique_ptr<T> pPtr, Tcopier&& pCopier) noexcept :
		mPtr(std::move(pPtr)),
		mCopier(std::move(pCopier))
	{}
	copyable_ptr(copyable_ptr&&) noexcept = default;
	copyable_ptr(const copyable_ptr& pOther)
	{
		if (pOther.valid())
		{
			mPtr = pOther.mCopier(pOther.mPtr);
			mCopier = pOther.mCopier;
		}
	}

	copyable_ptr& operator=(copyable_ptr&&) noexcept = default;
	copyable_ptr& operator=(const copyable_ptr&) = default;
	
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
		return mPtr != nullptr && mCopier;
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
