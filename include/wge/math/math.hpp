#pragma once

#include <cstddef>
#include <cmath>
#include <type_traits>
#include <string>

namespace wge::math
{

static constexpr float pi = 3.14159265f;
static constexpr float deg_max = 360.f;
static constexpr float deg_half = 180.f;

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
inline constexpr const T& max(const T& pL, const T& pR) noexcept
{
	return pL > pR ? pL : pR;
}

template <typename T>
inline constexpr const T& min(const T& pL, const T& pR) noexcept
{
	return pL < pR ? pL : pR;
}

template <typename T>
inline constexpr const T& clamp(const T& pVal, const T& pMin, const T& pMax) noexcept
{
	return min(max(pVal, pMin), pMax);
}

// Template for normalizing a value.
// Specialization is required.
template<typename T>
inline T normalize(const T& a) noexcept
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

class degrees;

class radians
{
public:
	radians() noexcept;
	radians(float pRadians) noexcept;
	radians(const degrees& pDegrees) noexcept;

	degrees to_degrees() const noexcept;

	float value() const noexcept;
	operator float() const noexcept
	{
		return mRadians;
	}

	// Operations
	radians operator + (const radians& pRadians) const noexcept;
	radians operator - (const radians& pRadians) const noexcept;
	radians operator * (const radians& pRadians) const noexcept;
	radians operator / (const radians& pRadians) const noexcept;

	radians operator - () const noexcept;

	// Assignments
	radians& operator = (const radians& pRadians) noexcept;
	radians& operator += (const radians& pRadians) noexcept;
	radians& operator -= (const radians& pRadians) noexcept;
	radians& operator *= (const radians& pRadians) noexcept;
	radians& operator /= (const radians& pRadians) noexcept;

private:
	float mRadians;
};

class degrees
{
public:
	degrees() noexcept;
	degrees(float pDegrees) noexcept;
	degrees(const radians& pRadians) noexcept;

	radians to_radians() const noexcept;

	float value() const noexcept;
	operator float() const noexcept
	{
		return mDegrees;
	}

	// Operations
	degrees operator + (const degrees& pDegrees) const noexcept;
	degrees operator - (const degrees& pDegrees) const noexcept;
	degrees operator * (const degrees& pDegrees) const noexcept;
	degrees operator / (const degrees& pDegrees) const noexcept;

	degrees operator - () const noexcept;

	// Assignments
	degrees& operator = (const degrees& pDegrees) noexcept;
	degrees& operator += (const degrees& pDegrees) noexcept;
	degrees& operator -= (const degrees& pDegrees) noexcept;
	degrees& operator *= (const degrees& pDegrees) noexcept;
	degrees& operator /= (const degrees& pDegrees) noexcept;

private:
	float mDegrees;
};

degrees radians_to_degrees(float pRadians) noexcept;
radians degrees_to_radians(float pDegrees) noexcept;

inline float sin(const radians& pRadians) noexcept
{
	return std::sinf(pRadians);
}

inline float cos(const radians& pRadians) noexcept
{
	return std::cosf(pRadians);
}

} // namespace wge::math

inline wge::math::degrees operator""_deg(long double pDegrees) noexcept
{
	return static_cast<float>(pDegrees);
}

inline wge::math::degrees operator""_deg(std::uintmax_t pDegrees) noexcept
{
	return static_cast<float>(pDegrees);
}

inline wge::math::radians operator""_rad(long double pRadians) noexcept
{
	return static_cast<float>(pRadians);
}
