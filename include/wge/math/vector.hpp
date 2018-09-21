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
	union{
		struct{
			float x, y;
		};
		float components[2];
	};

public:
	constexpr vec2() :
		x(0),
		y(0)
	{}
	constexpr vec2(float pX, float pY) :
		x(pX),
		y(pY)
	{}
	constexpr vec2(const vec2& pCopy) :
		x(pCopy.x),
		y(pCopy.y)
	{}

	float magnitude() const;

	float distance(const vec2& pTo) const;

	vec2& rotate(const radians& pRadians);

	vec2& normalize();

	vec2& abs();

	vec2& floor();
	vec2& floor_magnitude();
	vec2& ceil();
	vec2& ceil_magnitude();
	vec2& round();
	vec2& round_magnitude();

	// Vector operations
	vec2 operator + (const vec2& pR) const;
	vec2 operator - (const vec2& pR) const;
	vec2 operator * (const vec2& pR) const;
	vec2 operator / (const vec2& pR) const;

	vec2 operator - () const;

	// Scalar operations
	vec2 operator * (const float& pR) const;
	vec2 operator / (const float& pR) const;

	// Vector assignments
	vec2& operator = (const vec2& pR);
	vec2& operator += (const vec2& pR);
	vec2& operator -= (const vec2& pR);
	vec2& operator *= (const vec2& pR);
	vec2& operator /= (const vec2& pR);

	// Scalar assignments
	vec2& operator *= (const float& pR);
	vec2& operator /= (const float& pR);

	// Returns format "([x], [y])"
	std::string to_string() const;

	template <typename Tx, typename Ty>
	constexpr vec2 swizzle(Tx pX, Ty pY) const
	{
		return math::swizzle(*this, pX, pY);
	}
};

template <typename Tx, typename Ty>
constexpr vec2 swizzle(const vec2& pVec, Tx pX, Ty pY)
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
inline vec2 normalize<vec2>(const vec2& a)
{
	return vec2(a).normalize();
}

template<>
inline vec2 abs<vec2>(const vec2& a)
{
	return vec2(a).abs();
}

template<>
inline vec2 floor<vec2>(const vec2& a)
{
	return vec2(a).floor();
}

template<>
inline vec2 ceil<vec2>(const vec2& a)
{
	return vec2(a).ceil();
}

template<>
inline vec2 round<vec2>(const vec2& a)
{
	return vec2(a).round();
}

} // namespace math
} // namespace wge