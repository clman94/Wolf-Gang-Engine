#pragma once

#include <cstddef>
#include <cmath>
#include <type_traits>
#include <string>

namespace wge
{
namespace math
{

static constexpr float pi = 3.14159265f;

template<class T>
inline T mod(const T& a, const T& b)
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

template<class T>
inline T abs(const T& a)
{
	return std::abs(a);
}

template<class T>
inline T sqrt(const T& a)
{
	return std::sqrt(a);
}

template<class T>
inline T pow(const T& a, const T& b)
{
	return std::pow(a, b);
}

template<class T>
inline T floor(const T& a)
{
	return std::floor(a);
}

template<class T>
inline T ceil(const T& a)
{
	return std::ceil(a);
}

template<class T>
inline T round(const T& a)
{
	return std::round(a);
}

template <typename T>
inline constexpr const T& max(const T& pL, const T& pR)
{
	return pL > pR ? pL : pR;
}

template <typename T>
inline constexpr const T& min(const T& pL, const T& pR)
{
	return pL < pR ? pL : pR;
}


// Template for normalizing a value.
// Specialization is required.
template<class T>
inline T normalize(const T& a)
{
	static_assert(false, "Invalid normalize() template");
	return a;
}

// Returns an always positive value if b > 0.
// This is unlike the '%' operator (and std::fmodf) that only returns a remainder
// that can be negative if a is negative.
template<class T>
inline T positive_modulus(const T& a, const T& b)
{
	return mod(mod(a, b) + b, b);
}

// Calculate the binomial coefficient
unsigned long binomial_coeff(unsigned long n, unsigned long k);

class degrees;

class radians
{
public:
	radians();
	radians(const float& pRadians);
	radians(const radians& pRadians);
	radians(const degrees& pDegrees);

	degrees to_degrees() const;

	float value() const;
	operator float() const
	{
		return mRadians;
	}

	// Operations
	radians operator + (const radians& pRadians) const;
	radians operator - (const radians& pRadians) const;
	radians operator * (const radians& pRadians) const;
	radians operator / (const radians& pRadians) const;

	// Assignments
	radians& operator = (const radians& pRadians);
	radians& operator += (const radians& pRadians);
	radians& operator -= (const radians& pRadians);
	radians& operator *= (const radians& pRadians);
	radians& operator /= (const radians& pRadians);

private:
	float mRadians;
};

class degrees
{
public:
	degrees();
	degrees(const float& pDegrees);
	degrees(const degrees& pDegrees);
	degrees(const radians& pRadians);

	radians to_radians() const;

	float value() const;
	operator float() const
	{
		return mDegrees;
	}

	// Operations
	degrees operator + (const degrees& pDegrees) const;
	degrees operator - (const degrees& pDegrees) const;
	degrees operator * (const degrees& pDegrees) const;
	degrees operator / (const degrees& pDegrees) const;

	// Assignments
	degrees& operator = (const degrees& pDegrees);
	degrees& operator += (const degrees& pDegrees);
	degrees& operator -= (const degrees& pDegrees);
	degrees& operator *= (const degrees& pDegrees);
	degrees& operator /= (const degrees& pDegrees);

private:
	float mDegrees;
};

degrees radians_to_degrees(float pRadians);
radians degrees_to_radians(float pDegrees);

} // namespace math
} // namespace wge

inline wge::math::degrees operator""_deg(long double pDegrees)
{
	return static_cast<float>(pDegrees);
}

inline wge::math::radians operator""_rad(long double pRadians)
{
	return static_cast<float>(pRadians);
}
