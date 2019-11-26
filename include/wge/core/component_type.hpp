#pragma once

#include <wge/util/strongly_typed_id.hpp>
#include <cstdint>

namespace wge::core
{

class family
{
	static inline std::size_t counter = 1;

public:
	constexpr family() noexcept = default;
	constexpr family(std::size_t pIndex) noexcept :
		mIndex(pIndex)
	{}

	template <typename T>
	static constexpr family from() noexcept
	{
		static std::size_t index = counter++;
		return{ index };
	}

	constexpr bool operator<(const family& pR) const noexcept
	{
		return mIndex < pR.mIndex;
	}

	constexpr bool operator<=(const family& pR) const noexcept
	{
		return mIndex <= pR.mIndex;
	}

	constexpr bool operator>(const family& pR) const noexcept
	{
		return mIndex > pR.mIndex;
	}

	constexpr bool operator>=(const family& pR) const noexcept
	{
		return mIndex >= pR.mIndex;
	}

	constexpr bool operator==(const family& pR) const noexcept
	{
		return mIndex == pR.mIndex;
	}

	constexpr bool operator!=(const family& pR) const noexcept
	{
		return mIndex != pR.mIndex;
	}

private:
	std::size_t mIndex = 0;
};

// TODO: Fully implement.
using bucket = std::size_t;
constexpr inline bucket default_bucket = 0;

struct component_type
{
public:
	constexpr component_type() noexcept = default;
	constexpr component_type(const family& pFamily, bucket pBucket) noexcept :
		mFamily(pFamily),
		mBucket(pBucket)
	{}

	template <typename T>
	constexpr static component_type from(bucket pBucket = default_bucket) noexcept
	{
		return component_type{ family::from<T>(), pBucket };
	}

	constexpr bool operator<(const component_type& pR) const noexcept
	{
		return std::pair(mFamily, mBucket) < std::pair(pR.mFamily, pR.mBucket);
	}

	constexpr bool operator==(const component_type& pR) const noexcept
	{
		return mFamily == pR.mFamily && mBucket == pR.mBucket;
	}

	constexpr bool operator!=(const component_type& pR) const noexcept
	{
		return !operator==(pR);
	}

private:
	family mFamily;
	bucket mBucket = default_bucket;
};

} // namespace wge::core
