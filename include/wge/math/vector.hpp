#pragma once

#include <wge/math/math.hpp>

namespace wge
{
namespace math
{
// Placeholders for swizzling
constexpr struct _x_t {} _x;
constexpr struct _y_t {} _y;

class vec2
{
public:
	union {
		struct {
			float x, y;
		};
		float components[2];
	};

public:
	constexpr vec2() noexcept :
		x(0),
		y(0)
	{}
	constexpr vec2(float pX, float pY) noexcept :
		x(pX),
		y(pY)
	{}
	constexpr vec2(const vec2&) noexcept = default;

	float magnitude() const noexcept;

	float distance(const vec2& pTo) const noexcept;

	vec2& rotate(const radians& pRadians) noexcept;

	vec2& normalize() noexcept;

	vec2& abs() noexcept;

	vec2& floor() noexcept;
	vec2& floor_magnitude() noexcept;
	vec2& ceil() noexcept;
	vec2& ceil_magnitude() noexcept;
	vec2& round() noexcept;
	vec2& round_magnitude() noexcept;

	vec2& swap_xy() noexcept;

	// Vector operations
	vec2 operator + (const vec2& pR) const noexcept;
	vec2 operator - (const vec2& pR) const noexcept;
	vec2 operator * (const vec2& pR) const noexcept;
	vec2 operator / (const vec2& pR) const noexcept;

	vec2 operator - () const noexcept;

	// Scalar operations
	vec2 operator * (const float& pR) const noexcept;
	vec2 operator / (const float& pR) const noexcept;

	// Vector assignments
	vec2& operator = (const vec2& pR) noexcept;
	vec2& operator += (const vec2& pR) noexcept;
	vec2& operator -= (const vec2& pR) noexcept;
	vec2& operator *= (const vec2& pR) noexcept;
	vec2& operator /= (const vec2& pR) noexcept;

	// Scalar assignments
	vec2& operator *= (const float& pR) noexcept;
	vec2& operator /= (const float& pR) noexcept;

	bool operator == (const vec2& pR) const noexcept;
	bool operator != (const vec2& pR) const noexcept;

	// Returns format "([x], [y])"
	std::string to_string() const;

	template <typename Tx, typename Ty>
	constexpr vec2 swizzle(Tx pX, Ty pY) const noexcept
	{
		return math::swizzle(*this, pX, pY);
	}
};

inline constexpr vec2 operator * (float pVal, const math::vec2& pVec) noexcept
{
	return{ pVal * pVec.x , pVal * pVec.y };
}

inline constexpr vec2 operator / (float pVal, const math::vec2& pVec) noexcept
{
	return{ pVal / pVec.x , pVal / pVec.y };
}

template <typename Tx, typename Ty>
inline constexpr vec2 swizzle(const vec2& pVec, Tx pX, Ty pY) noexcept
{
	vec2 result;

	if constexpr (std::is_same<Tx, _x_t>::value)
		result.x = pVec.x;
	else if constexpr (std::is_same<Tx, _y_t>::value)
		result.x = pVec.y;
	else
		result.x = static_cast<float>(pX);

	if constexpr (std::is_same<Ty, _x_t>::value)
		result.y = pVec.x;
	else if constexpr (std::is_same<Ty, _y_t>::value)
		result.y = pVec.y;
	else
		result.y = static_cast<float>(pY);

	return result;
}

template<>
inline vec2 normalize<vec2>(const vec2& a) noexcept
{
	return vec2(a).normalize();
}

template<>
inline vec2 abs<vec2>(const vec2& a) noexcept
{
	return vec2(a).abs();
}

template<>
inline vec2 floor<vec2>(const vec2& a) noexcept
{
	return vec2(a).floor();
}

template<>
inline vec2 ceil<vec2>(const vec2& a) noexcept
{
	return vec2(a).ceil();
}

template<>
inline vec2 round<vec2>(const vec2& a) noexcept
{
	return vec2(a).round();
}

inline vec2 swap_xy(const vec2& a) noexcept
{
	return vec2(a).swap_xy();
}


template <>
inline auto max<vec2>(const vec2& pL, const vec2& pR) noexcept
{
	return vec2{ max(pL.x, pR.x), max(pL.y, pR.y) };
}

template <>
inline auto min<vec2>(const vec2& pL, const vec2& pR) noexcept
{
	return vec2{ min(pL.x, pR.x), min(pL.y, pR.y) };
}

} // namespace math
} // namespace wge