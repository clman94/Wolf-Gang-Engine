#pragma once

#include <type_traits>
#include <iterator>
#include <cassert>

namespace wge::util
{

template <typename T>
class span;

namespace detail
{

template <typename T>
struct is_span : std::false_type {};

template <typename T>
struct is_span<span<T>> : std::true_type {};

template<typename, typename = std::void_t<>>
struct has_data_and_size : std::false_type {};

template <typename T>
struct has_data_and_size<T, std::void_t<
	decltype(std::data(T{})),
	decltype(std::size(T{}))>> : std::true_type {};

template <typename T, typename U = std::remove_cv_t<std::remove_reference_t<T>>>
struct is_container
{
	static constexpr bool value = !is_span<U>::value &&
		!std::is_array<U>::value && has_data_and_size<U>::value;
};

} // namespace detail

// Loosely based on std::span from c++20.
// Because C++20 is not official yet, I'm creating my own
// version for my own needs.
// This is a dynamic_extent version only.
template <typename T>
class span
{
public:
	using type = T;
	using pointer = T*;
	using const_pointer = const T*;
	using reference = T&;
	using element_type = std::remove_cv_t<T>;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using iterator = pointer;
	using const_iterator = const_pointer;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
	constexpr span() noexcept {}

	constexpr span(pointer pPtr, size_type pSize) noexcept :
		mPtr(pPtr),
		mSize(pSize)
	{}

	constexpr span(pointer pPtr_first, pointer pPtr_last) noexcept :
		mPtr(pPtr_first),
		mSize(pPtr_last - pPtr_first)
	{}

	template <typename Telement, std::size_t N,
		typename = std::enable_if_t<
			std::is_same_v<element_type, std::remove_cv_t<Telement>>>>
	constexpr span(Telement(&pArr)[N]) noexcept :
		mPtr(pArr),
		mSize(N)
	{}

	template <typename Tcontainer,
		typename = std::enable_if_t<detail::is_container<Tcontainer>::value>>
	constexpr span(Tcontainer& pContainer) noexcept :
		mPtr(std::data(pContainer)),
		mSize(std::size(pContainer))
	{}

	template <typename Tcontainer,
		typename = std::enable_if_t<detail::is_container<Tcontainer>::value ||
			// This allows us to reuse this overload for converting
			// non-const spans into const spans.
			(detail::is_span<Tcontainer>::value &&
				!std::is_const_v<typename Tcontainer::type> &&
				std::is_const_v<T>)>>
	constexpr span(const Tcontainer& pContainer) noexcept :
		mPtr(std::data(pContainer)),
		mSize(std::size(pContainer))
	{}

	constexpr span(const span&) noexcept = default;

	constexpr iterator begin() const noexcept
	{
		return mPtr;
	}

	constexpr iterator end() const noexcept
	{
		return mPtr + mSize;
	}

	constexpr const_iterator cbegin() const noexcept
	{
		return mPtr;
	}

	constexpr const_iterator cend() const noexcept
	{
		return mPtr + mSize;
	}

	constexpr reverse_iterator rbegin() const noexcept
	{
		return reverse_iterator{ mPtr };
	}

	constexpr reverse_iterator rend() const noexcept
	{
		return reverse_iterator{ mPtr + mSize };
	}

	constexpr const_reverse_iterator crbegin() const noexcept
	{
		return const_reverse_iterator{ mPtr };
	}

	constexpr const_reverse_iterator crend() const noexcept
	{
		return const_reverse_iterator{ mPtr + mSize };
	}

	constexpr pointer data() const noexcept
	{
		return mPtr;
	}

	constexpr reference front() const noexcept
	{
		assert(!empty());
		return *mPtr;
	}

	constexpr reference back() const noexcept
	{
		assert(!empty());
		return at(mSize - 1);
	}

	constexpr reference at(std::size_t pIndex) const noexcept
	{
		assert(!empty());
		assert(pIndex < mSize);
		return mPtr[pIndex];
	}

	constexpr reference operator[](std::size_t pIndex) const noexcept
	{
		return at(pIndex);
	}

	constexpr std::size_t size() const noexcept
	{
		return mSize;
	}

	constexpr std::size_t size_bytes() const noexcept
	{
		return mSize * sizeof(element_type);
	}

	constexpr span subspan(std::size_t pOffset,
		std::size_t pCount = std::numeric_limits<std::size_t>::max()) const noexcept
	{
		if (empty() || pOffset >= mSize)
			return{};
		pOffset = std::min(pOffset, mSize - 1);
		pCount = std::min(pCount, mSize - pOffset);
		return span{ mPtr + pOffset, pCount };
	}

	constexpr span first(std::size_t pCount) const noexcept
	{
		return subspan(0, pCount);
	}

	constexpr span last(std::size_t pCount) const noexcept
	{
		return subspan(mSize - std::min(pCount, mSize));
	}

	constexpr bool empty() const noexcept
	{
		return mSize == 0;
	}

private:
	pointer mPtr = nullptr;
	std::size_t mSize = 0;
};

// Deduction guides

template <typename T>
span(T*, std::size_t)->span<T>;

template <typename T>
span(T*, T*)->span<T>;

template <typename T, std::size_t N>
span(T(&pArr)[N])->span<T>;

template <typename Tcontainer>
span(Tcontainer&)->span<typename Tcontainer::value_type>;

template <typename Tcontainer>
span(const Tcontainer&)->span<const typename Tcontainer::value_type>;

} // namespace wge::util
