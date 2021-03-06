#pragma once

#include <cstddef>
#include <cmath>
#include <type_traits>
#include <string>
#include <limits>

namespace wge::math
{

namespace detail
{

template <typename T>
using only_floating_point = typename std::enable_if_t<std::is_floating_point_v<T>>;

template <typename T>
using only_integral = typename std::enable_if_t<std::is_integral_v<T>>;

template <typename T>
using only_arithmetic = typename std::enable_if_t<std::is_arithmetic_v<T>>;

template <typename T>
using only_signed = typename std::enable_if_t<std::is_floating_point_v<T> || std::is_signed_v<T>>;

} // namespace detail

constexpr float pi = 3.14159265f;
constexpr float deg_max = 360.f;
constexpr float deg_half = 180.f;
constexpr float rad_max = pi * 2;
constexpr float rad_half = pi;

template <typename T>
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

template <typename T, typename = detail::only_floating_point<T>>
inline T exp(const T& a) noexcept
{
	return std::exp(a);
}

template <typename T, typename = detail::only_floating_point<T>>
inline T exp2(const T& a) noexcept
{
	return std::exp2(a);
}

template <typename T, typename = detail::only_floating_point<T>>
inline T log(const T& a) noexcept
{
	return std::log(a);
}

template <typename T, typename = detail::only_floating_point<T>>
inline T log2(const T& a) noexcept
{
	return std::log2(a);
}

template <typename T, typename = detail::only_floating_point<T>>
inline T log10(const T& a) noexcept
{
	return std::log10(a);
}

template <typename T, typename = detail::only_floating_point<T>>
inline T abs(const T& a) noexcept
{
	return std::abs(a);
}

template <typename T, typename = detail::only_floating_point<T>>
inline T sqrt(const T& a) noexcept
{
	return std::sqrt(a);
}

template <typename T, typename = detail::only_arithmetic<T>>
inline T pow(const T& a, const T& b) noexcept
{
	return std::pow(a, b);
}

template <typename T, typename = detail::only_arithmetic<T>>
inline T floor(const T& a) noexcept
{
	if constexpr (std::is_floating_point_v<T>)
		return std::floor(a);
	else
		return a;
}

template <typename T, typename = detail::only_arithmetic<T>>
inline T ceil(const T& a) noexcept
{
	if constexpr (std::is_floating_point_v<T>)
		return std::ceil(a);
	else
		return a;
}

template<typename T, typename = detail::only_arithmetic<T>>
inline T round(const T& a) noexcept
{
	if constexpr (std::is_floating_point_v<T>)
		return std::round(a);
	else
		return a;
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

template <typename T, typename Talpha>
inline constexpr T lerp(const T& pA, const T& pB, Talpha pAlpha) noexcept
{
	return (pB - pA) * pAlpha + pA;
}

template <typename T>
inline constexpr T clamp(const T& pVal, const T& pMin, const T& pMax) noexcept
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
template <typename T>
inline T normal(const T& a) noexcept;

template <typename T>
inline bool is_nan(const T& a) noexcept
{
	if constexpr (std::is_floating_point_v<T>)
		return std::isnan(a);
	else
		return false;
}

// Returns an always positive value if b > 0.
// This is unlike the '%' operator (and std::fmodf) that only returns a remainder
// that can be negative if a is negative.
template <typename T>
inline T positive_modulus(const T& a, const T& b) noexcept
{
	return mod(mod(a, b) + b, b);
}

template <typename T>
constexpr T midpoint(const T& a, const T& b) noexcept
{
	return (a + b) / 2;
}

template <typename T>
constexpr T sign_impl(const T& x, std::false_type is_signed) {
	return static_cast<T>(T(0) < x);
}

template <typename T>
constexpr T sign_impl(const T& x, std::true_type is_signed) {
	return static_cast<T>((T(0) < x) - (x < T(0)));
}

template <typename T>
constexpr T sign(const T& x) {
	return sign_impl(x, std::is_signed<T>());
}

} // namespace wge::math
