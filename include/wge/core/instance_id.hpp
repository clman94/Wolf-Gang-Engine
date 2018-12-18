#pragma once

#include <cstdint>

namespace wge::core
{

using instance_id_t = std::uint64_t;

// Template for strongly typed id values.
template <typename>
class instance_id
{
public:
	constexpr instance_id() noexcept = default;
	constexpr instance_id(instance_id_t pVal) noexcept :
		mValue(pVal)
	{
	}

	constexpr bool operator<(const instance_id& pR) const noexcept
	{
		return mValue < pR.mValue;
	}

	constexpr bool operator==(const instance_id& pR) const noexcept
	{
		return mValue == pR.mValue;
	}
	
	constexpr instance_id_t get_value() const noexcept
	{
		return mValue;
	}

	constexpr void set_value(instance_id_t pId) noexcept
	{
		mValue = pId;
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
	instance_id_t mValue{ 0 };
};

class game_object;
// Represents the instance id of a game object
using object_id = instance_id<game_object>;

class component;
// Represents the instance id of a component
using component_id = instance_id<component>;

} // namespace wge::core
