#pragma once

#include <type_traits>

// Generate a set of operators for using an enum class for flags
#define ENUM_CLASS_FLAG_OPERATORS(A) \
	inline constexpr A operator | (const A& pL, const A& pR) noexcept           \
	{                                                                           \
		using type = std::underlying_type_t<A>;                                 \
		return static_cast<A>(static_cast<type>(pL) | static_cast<type>(pR));   \
	}                                                                           \
	                                                                            \
	inline constexpr bool operator & (const A& pL, const A& pR) noexcept        \
	{                                                                           \
		using type = std::underlying_type_t<A>;                                 \
		return (static_cast<type>(pL) & static_cast<type>(pR)) != 0;            \
	}                                                                           \
	                                                                            \
	inline constexpr bool operator ^ (const A& pL, const A& pR) noexcept        \
	{                                                                           \
		using type = std::underlying_type_t<A>;                                 \
		return (static_cast<type>(pL) ^ static_cast<type>(pR)) != 0;            \
	}

namespace wge::util
{

using flag_type = unsigned long;

} // namespace wge::util
