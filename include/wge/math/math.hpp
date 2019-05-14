#pragma once

#include <cstddef>
#include <cmath>
#include <type_traits>
#include <string>
#include <limits>

namespace wge::math
{

static constexpr float pi = 3.14159265f;
static constexpr float deg_max = 360.f;
static constexpr float deg_half = 180.f;
static constexpr float rad_max = pi * 2;
static constexpr float rad_half = pi;

template<typename T>
inline T mod(const T& a, const T& b) noexcept
{
	if constexpr (std::is_floating_point<T>::value)
	{
		return std::fmod(a, b);
	}
	else if constexpr (std::is_integral<T>::value)
	{
		return a % b;
	}
}

template<typename T>
inline T abs(const T& a) noexcept
{
	return std::abs(a);
}

template<typename T>
inline T sqrt(const T& a) noexcept
{
	return std::sqrt(a);
}

template<typename T>
inline T pow(const T& a, const T& b) noexcept
{
	return std::pow(a, b);
}

template<typename T>
inline T floor(const T& a) noexcept
{
	return std::floor(a);
}

template<typename T>
inline T ceil(const T& a) noexcept
{
	return std::ceil(a);
}

template<typename T>
inline T round(const T& a) noexcept
{
	return std::round(a);
}

template <typename T>
inline constexpr auto max(const T& pL, const T& pR) noexcept
{
	return pL > pR ? pL : pR;
}

template <typename T>
inline constexpr auto min(const T& pL, const T& pR) noexcept
{
	return pL < pR ? pL : pR;
}

template <typename T>
inline constexpr const T& clamp(const T& pVal, const T& pMin, const T& pMax) noexcept
{
	return min(max(pVal, pMin), pMax);
}

template <typename T, typename = std::enable_if_t<std::numeric_limits<T>::is_specialized>>
inline constexpr T almost_equal(const T& pL, const T& pR) noexcept
{
	T dif = pL - pR;
	return dif > -std::numeric_limits<T>::epsilon()
		&& dif < std::numeric_limits<T>::epsilon();
}

// Template for normalizing a value.
// Specialization is required.
template<typename T>
inline T normal(const T& a) noexcept
{
	static_assert(false, "Invalid normalize() instantiation");
	return a;
}

// Returns an always positive value if b > 0.
// This is unlike the '%' operator (and std::fmodf) that only returns a remainder
// that can be negative if a is negative.
template<typename T>
inline T positive_modulus(const T& a, const T& b) noexcept
{
	return mod(mod(a, b) + b, b);
}

} // namespace wge::math
