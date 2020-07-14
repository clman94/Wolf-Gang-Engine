#pragma once

#include <wge/util/span.hpp>
#include <cmath>

namespace wge::math
{

class degrees;

class radians
{
public:
	radians() noexcept;
	radians(float pRadians) noexcept;
	radians(const degrees& pDegrees) noexcept;

	util::span<float> components() noexcept
	{
		return util::span<float>(&mRadians, 1);
	}

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
	radians operator % (const radians& pRadians) const noexcept;

	radians operator - () const noexcept;

	// Assignments
	radians& operator = (const radians& pRadians) noexcept;
	radians& operator += (const radians& pRadians) noexcept;
	radians& operator -= (const radians& pRadians) noexcept;
	radians& operator *= (const radians& pRadians) noexcept;
	radians& operator /= (const radians& pRadians) noexcept;

	// Comparasons
	bool operator == (const radians& pRadians) const noexcept;
	bool operator != (const radians& pRadians) const noexcept;
	bool operator >= (const radians& pRadians) const noexcept;
	bool operator <= (const radians& pRadians) const noexcept;
	bool operator > (const radians& pRadians) const noexcept;
	bool operator < (const radians& pRadians) const noexcept;

private:
	float mRadians;
};

class degrees
{
public:
	degrees() noexcept;
	degrees(float pDegrees) noexcept;
	degrees(const radians& pRadians) noexcept;

	util::span<float> components() noexcept
	{
		return util::span<float>(&mDegrees, 1);
	}

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
	degrees operator % (const degrees& pDegrees) const noexcept;

	degrees operator - () const noexcept;

	// Assignments
	degrees& operator = (const degrees& pDegrees) noexcept;
	degrees& operator += (const degrees& pDegrees) noexcept;
	degrees& operator -= (const degrees& pDegrees) noexcept;
	degrees& operator *= (const degrees& pDegrees) noexcept;
	degrees& operator /= (const degrees& pDegrees) noexcept;

	// Comparasons
	bool operator == (const degrees& pDegrees) const noexcept;
	bool operator != (const degrees& pDegrees) const noexcept;
	bool operator >= (const degrees& pDegrees) const noexcept;
	bool operator <= (const degrees& pDegrees) const noexcept;
	bool operator > (const degrees& pDegrees) const noexcept;
	bool operator < (const degrees& pDegrees) const noexcept;

private:
	float mDegrees;
};

degrees terminal_angle(const degrees& pDegrees) noexcept;
radians terminal_angle(const radians& pDegrees) noexcept;

degrees radians_to_degrees(float pRadians) noexcept;
radians degrees_to_radians(float pDegrees) noexcept;

inline float sin(const radians& pRadians) noexcept
{
	return std::sin(pRadians);
}

inline float cos(const radians& pRadians) noexcept
{
	return std::cos(pRadians);
}

} // namespace wge::math

inline wge::math::degrees operator""_deg(long double pDegrees) noexcept
{
	return static_cast<float>(pDegrees);
}

inline wge::math::degrees operator""_deg(unsigned long long pDegrees) noexcept
{
	return static_cast<float>(pDegrees);
}

inline wge::math::radians operator""_rad(long double pRadians) noexcept
{
	return static_cast<float>(pRadians);
}

inline wge::math::radians operator""_rad(unsigned long long pRadians) noexcept
{
	return static_cast<float>(pRadians);
}
