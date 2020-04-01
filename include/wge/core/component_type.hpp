#pragma once

#include <wge/util/strongly_typed_id.hpp>
#include <cstdint>

namespace wge::core
{

class family
{
	static inline std::size_t counter = 1;

	template <typename T>
	static constexpr family from_impl() noexcept
	{
		static std::size_t index = counter++;
		return{ index };
	}

public:
	constexpr family() noexcept = default;
	constexpr family(std::size_t pIndex) noexcept :
		mIndex(pIndex)
	{}

	template <typename T>
	static constexpr family from() noexcept
	{
		return from_impl<std::remove_cv_t<std::remove_reference_t<T>>>();
	}

	constexpr bool operator<(const family& pR) const noexcept
	{
		return mIndex < pR.mIndex;
	}

	constexpr bool operator==(const family& pR) const noexcept
	{
		return mIndex == pR.mIndex;
	}

	constexpr bool operator!=(const family& pR) const noexcept
	{
		return mIndex != pR.mIndex;
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

private:
	std::size_t mIndex = 0;
};

using bucket = std::size_t;
constexpr inline bucket default_bucket = 0;

// Select a bucket in compile-time
template<typename T, bucket pBucket>
struct bselect
{
	using type = T;
	static constexpr bucket bucket = pBucket;
};

template <typename T>
struct bselect_adaptor
{
	using type = T;
	static constexpr bucket bucket = default_bucket;
	static constexpr bool is_bselect = false;
};

template <typename T, bucket pBucket>
struct bselect_adaptor<bselect<T, pBucket>>
{
	using type = T;
	static constexpr bucket bucket = pBucket;
	static constexpr bool is_bselect = true;
};

struct component_type
{
public:
	constexpr component_type() noexcept = default;
	constexpr component_type(const family& pFamily, bucket pBucket) noexcept :
		mFamily(pFamily),
		mBucket(pBucket)
	{}

	template <typename T>
	static constexpr component_type from(bucket pBucket = default_bucket) noexcept
	{
		// bselect should take precedence.
		using selector = bselect_adaptor<T>;
		return component_type{ family::from<selector::type>(), selector::is_bselect ? selector::bucket : pBucket };
	}

	constexpr bool operator==(const component_type& pR) const noexcept
	{
		return mFamily == pR.mFamily && mBucket == pR.mBucket;
	}

	constexpr bool operator!=(const component_type& pR) const noexcept
	{
		return !operator==(pR);
	}

	constexpr bool operator<(const component_type& pR) const noexcept
	{
		return mFamily < pR.mFamily || (mFamily == pR.mFamily && mBucket < pR.mBucket);
	}

	constexpr bool operator<=(const component_type& pR) const noexcept
	{
		return mFamily <= pR.mFamily || (mFamily == pR.mFamily && mBucket <= pR.mBucket);
	}

	constexpr bool operator>(const component_type& pR) const noexcept
	{
		return mFamily > pR.mFamily || (mFamily == pR.mFamily && mBucket > pR.mBucket);
	}

	constexpr bool operator>=(const component_type& pR) const noexcept
	{
		return mFamily >= pR.mFamily || (mFamily == pR.mFamily && mBucket >= pR.mBucket);
	}

	constexpr family get_family() const noexcept
	{
		return mFamily;
	}

	constexpr bucket get_bucket() const noexcept
	{
		return mBucket;
	}

private:
	family mFamily;
	bucket mBucket = default_bucket;
};

} // namespace wge::core
