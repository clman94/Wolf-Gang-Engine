#pragma once

#include <wge/math/math.hpp>

namespace wge
{
namespace math
{

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
	vec2() :
		x(0),
		y(0)
	{}
	vec2(float pX, float pY) :
		x(pX),
		y(pY)
	{}
	vec2(const vec2& pCopy) :
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
	vec2& ciel_magnitude();
	vec2& round();
	vec2& round_magnitude();

	// Vector operations
	vec2 operator + (const vec2& pR) const;
	vec2 operator - (const vec2& pR) const;
	vec2 operator * (const vec2& pR) const;
	vec2 operator / (const vec2& pR) const;

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

	std::string to_string() const;
};

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