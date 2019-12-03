#pragma once

namespace wge::util
{

// Template for strongly typed id values.
// These values don't need most arithmetic operators so they are not included.
// A value of 0 is considered an invalid id so is_valid() will return false if the value is indeed 0.
template <typename, typename Tvalue>
class strongly_typed_id
{
public:
	constexpr strongly_typed_id() noexcept = default;
	explicit constexpr strongly_typed_id(const Tvalue& pVal) noexcept :
		mValue(pVal)
	{}

	constexpr bool operator<(const strongly_typed_id& pR) const noexcept
	{
		return mValue < pR.mValue;
	}

	constexpr bool operator==(const strongly_typed_id& pR) const noexcept
	{
		return mValue == pR.mValue;
	}

	constexpr bool operator!=(const strongly_typed_id& pR) const noexcept
	{
		return mValue != pR.mValue;
	}

	constexpr const Tvalue& get_value() const noexcept
	{
		return mValue;
	}

	constexpr void set_value(const Tvalue& pValue) noexcept
	{
		mValue = pValue;
	}

	constexpr bool is_valid() const noexcept
	{
		return mValue != 0;
	}

	constexpr operator bool() const noexcept
	{
		return is_valid();
	}

	constexpr void reset() noexcept
	{
		mValue = 0;
	}

private:
	Tvalue mValue{ 0 };
};

} // namespace wge::util
