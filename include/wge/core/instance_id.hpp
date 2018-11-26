#pragma once

#include <cstdint>

namespace wge::core
{

using instance_id_t = std::uint64_t;

class instance_id
{
public:
	constexpr instance_id() = default;
	constexpr instance_id(instance_id_t pVal) :
		mValue(pVal)
	{
	}

	constexpr bool operator<(const instance_id& pR) const
	{
		return mValue < pR.mValue;
	}

	constexpr bool operator==(const instance_id& pR) const
	{
		return mValue == pR.mValue;
	}
	
	constexpr instance_id_t get_value() const
	{
		return mValue;
	}

	constexpr void set_value(instance_id_t pId)
	{
		mValue = pId;
	}

	constexpr bool is_valid() const
	{
		return mValue != 0;
	}

	constexpr operator bool() const
	{
		return is_valid();
	}

	constexpr void reset()
	{
		mValue = 0;
	}

private:
	instance_id_t mValue{ 0 };
};

} // namespace wge::core
