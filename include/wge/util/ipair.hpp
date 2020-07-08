#pragma once

#include "ptr_adaptor.hpp"

#include <iterator>
#include <tuple>

namespace wge::util
{

template <typename Ttuple, std::size_t...I>
constexpr auto forward_tuple_elements_impl(Ttuple&& pTuple, std::index_sequence<I...>)
{
	return std::forward_as_tuple(std::get<I>(std::forward<Ttuple>(pTuple))...);
}

// Forward all elements from one tuple into another.
template <typename Ttuple>
constexpr auto forward_tuple_elements(Ttuple&& pTuple)
{
	return forward_tuple_elements_impl(
		std::forward<Ttuple>(pTuple),
		std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Ttuple>>>{});
}

// Convert a tuple into a pair.
template <typename Ttuple, typename = std::enable_if_t<std::tuple_size<Ttuple>::value >= 2>>
constexpr auto as_pair(Ttuple&& pTuple)
{
	return std::pair<std::tuple_element_t<0, Ttuple>, std::tuple_element_t<1, Ttuple>>(
		std::get<0>(std::forward<Ttuple>(pTuple)),
		std::get<1>(std::forward<Ttuple>(pTuple)));
}

template <typename Ttuple>
using first_element = std::tuple_element_t<0, Ttuple>;

struct pair_mapping_policy
{
	template <typename T>
	static constexpr auto map(std::size_t pIndex, T& pValue)
	{
		return as_pair(std::tuple_cat(std::make_tuple(pIndex), std::tie(pValue)));
	}
};

struct flatten_mapping_policy
{
	template <typename T>
	static constexpr auto map(std::size_t pIndex, T& pValue)
	{
		return std::tuple_cat(std::make_tuple(pIndex), forward_tuple_elements(pValue));
	}
};

// This is a janky utility for creating index-value pairs for
// containers.
// Example:
//   std::array arr = { "Hello", " " , "world" };
//   for (auto [index, value] : util::enumerate{ arr })
//     std::cout << index << " " << value << "\n";
//
template <typename T, typename Tmapping_policy>
class basic_enumerate
{
public:
	template <typename Titer>
	class iterator
	{
	public:
		constexpr iterator(const Titer& pIter) :
			mIter(pIter)
		{}

		constexpr bool operator==(const iterator& pIter) const
		{
			return mIter == pIter.mIter;
		}

		constexpr bool operator!=(const iterator& pIter) const
		{
			return mIter != pIter.mIter;
		}

		constexpr iterator& operator++()
		{
			next();
			return *this;
		}

		constexpr iterator operator++(int)
		{
			auto temp = *this;
			next();
			return temp;
		}

		constexpr void next()
		{
			++mIter;
			++mIndex;
		}

		constexpr auto get() const noexcept
		{
			return Tmapping_policy::map(mIndex, *mIter);
		}

		constexpr auto operator->() const noexcept
		{
			return util::ptr_adaptor{ get() };
		}

		constexpr auto operator*() const noexcept
		{
			return get();
		}

	private:
		std::size_t mIndex{ 0 };
		Titer mIter;
	};

	template <typename Titer>
	iterator(const Titer&)->iterator<Titer>;

	constexpr basic_enumerate(T&& pContainer) :
		mContainer(std::forward<T>(pContainer))
	{}

	constexpr auto begin() const
	{
		return iterator{ std::begin(*mContainer) };
	}

	constexpr auto end() const
	{
		return iterator{ std::end(*mContainer) };
	}

private:
	util::ptr_adaptor<T> mContainer;
};

template <typename T>
struct enumerate :
	basic_enumerate<T, pair_mapping_policy>
{
	template <typename T>
	constexpr enumerate(T&& pContainer) :
		basic_enumerate(std::forward<T>(pContainer))
	{}
};

template <typename T>
enumerate(T&& pContainer)->enumerate<T>;

template <typename T>
struct enumerate_tuple :
	basic_enumerate<T, flatten_mapping_policy>
{
	constexpr enumerate_tuple(T&& pContainer) :
		basic_enumerate(std::forward<T>(pContainer))
	{}
};

template <typename T>
enumerate_tuple(T&& pContainer)->enumerate_tuple<T>;

} // namespace wge::util
