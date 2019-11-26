#pragma once

#include <iterator>
#include "ptr_adaptor.hpp"

namespace wge::util
{

// This is a janky utility for creating index-value pairs for
// containers.
// Example:
//   std::array arr = { "Hello", " " , "world" };
//   for (auto [index, value] : util::ipair{ arr })
//     std::cout << index << " " << value << "\n";
//
template <typename T>
class ipair
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
			return std::pair<std::size_t, typename std::iterator_traits<Titer>::reference>{ mIndex, *mIter };
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

	constexpr ipair(T& pContainer) :
		mContainer(&pContainer)
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
	T* mContainer;
};

} // namespace wge::util
