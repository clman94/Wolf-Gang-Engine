#pragma once

namespace wge::util
{

// This is a janky utility for creating index-value pairs for
// containers. Should only be used in for-each loops.
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
		iterator(const Titer& pIter) :
			mIter(pIter)
		{}

		bool operator==(const iterator& pIter) const
		{
			return mIter == pIter.mIter;
		}

		bool operator!=(const iterator& pIter) const
		{
			return mIter != pIter.mIter;
		}

		iterator& operator++()
		{
			++mIter;
			++mIndex;
			return *this;
		}

		auto operator*() const noexcept
		{
			return std::pair<std::size_t, decltype(mIter.operator*())>{ mIndex, *mIter };
		}

	private:
		std::size_t mIndex{ 0 };
		Titer mIter;
	};

	ipair(T& pContainer) :
		mContainer(&pContainer)
	{}

	auto begin() const
	{
		return iterator<decltype(mContainer->begin())>(mContainer->begin());
	}

	auto end() const
	{
		return iterator<decltype(mContainer->end())>(mContainer->end());
	}

private:
	T* mContainer;
};

} // namespace wge::util
