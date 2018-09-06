#include <wge/math/math.hpp>
#include <wge/math/vector.hpp>

using namespace wge::math;

float vec2::magnitude() const
{
	return math::sqrt(x * x + y * y);
}

float vec2::distance(const vec2 & pTo) const
{
	return (pTo - *this).magnitude();
}

vec2& vec2::rotate(const radians& pRadians)
{
	float s = std::sinf(pRadians);
	float c = std::cosf(pRadians);
	float tx = x, ty = y;
	x = tx * c - ty * s;
	y = tx * s + ty * c;
	return *this;
}

vec2& vec2::normalize()
{
	float mag = magnitude();
	if (mag != 0)
	{
		x /= mag;
		y /= mag;
	}
	return *this;
}

vec2& vec2::abs()
{
	x = math::abs(x);
	y = math::abs(y);
	return *this;
}

vec2& vec2::floor()
{
	x = math::floor(x);
	y = math::floor(y);
	return *this;
}

vec2& vec2::floor_magnitude()
{
	float mag = magnitude();
	*this *= math::floor(mag) / mag;
	return *this;
}

vec2& vec2::ceil()
{
	x = math::ceil(x);
	y = math::ceil(y);
	return *this;
}

vec2& vec2::ciel_magnitude()
{
	float mag = magnitude();
	*this *= math::ceil(mag) / mag;
	return *this;
}

vec2& vec2::round()
{
	x = math::round(x);
	y = math::round(y);
	return *this;
}

vec2& vec2::round_magnitude()
{
	float mag = magnitude();
	*this *= math::round(mag) / mag;
	return *this;
}

vec2 vec2::operator + (const vec2& pR) const
{
	return { x + pR.x, y + pR.y };
}

vec2 vec2::operator - (const vec2& pR) const
{
	return { x - pR.x, y - pR.y };
}

vec2 vec2::operator * (const vec2& pR) const
{
	return { x * pR.x, y * pR.y };
}

vec2 vec2::operator / (const vec2& pR) const
{
	return { x / pR.x, y / pR.y };
}

vec2 vec2::operator * (const float& pR) const
{
	return { x * pR, y * pR };
}

vec2 vec2::operator / (const float& pR) const
{
	return { x / pR, y / pR };
}

vec2& vec2::operator = (const vec2& pR)
{
	x = pR.x;
	y = pR.y;
	return *this;
}

vec2& wge::math::vec2::operator += (const vec2& pR)
{
	x += pR.x;
	y += pR.y;
	return *this;
}

vec2& wge::math::vec2::operator -= (const vec2& pR)
{
	x -= pR.x;
	y -= pR.y;
	return *this;
}

vec2& wge::math::vec2::operator *= (const vec2& pR)
{
	x *= pR.x;
	y *= pR.y;
	return *this;
}

vec2& wge::math::vec2::operator /= (const vec2& pR)
{
	x /= pR.x;
	y /= pR.y;
	return *this;
}

vec2& vec2::operator *= (const float& pR)
{
	x *= pR;
	y *= pR;
	return *this;
}

vec2& vec2::operator /= (const float& pR)
{
	x /= pR;
	y /= pR;
	return *this;
}

std::string vec2::to_string() const
{
	return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
}

// TODO: Create a lookup table
// Reference: https://en.wikipedia.org/wiki/Binomial_coefficient
unsigned long wge::math::binomial_coeff(unsigned long n, unsigned long k)
{
	// Symmetry
	if (k > n - k)
		k = n - k;

	unsigned long c = 1;
	for (unsigned long i = 1; i <= k; i++, n--)
	{
		// Return 0 on overflow
		if (c / i > UINT_MAX / n)
			return 0;
		c = c / i * n + c % i * n / i;
	}

	return c;
}

degrees wge::math::radians_to_degrees(float pRadians)
{
	return{ radians{ pRadians } };
}

radians wge::math::degrees_to_radians(float pDegrees)
{
	return{ degrees{ pDegrees } };
}

radians::radians()
{
	mRadians = 0;
}

radians::radians(const float & pRadians)
{
	mRadians = pRadians;
}

radians::radians(const radians & pRadians)
{
	mRadians = pRadians.mRadians;
}

radians::radians(const degrees & pDegrees)
{
	mRadians = pDegrees.to_radians().value();
}

degrees radians::to_degrees() const
{
	return{ (mRadians / math::pi) * 180.f };
}

float radians::value() const
{
	return mRadians;
}

radians radians::operator + (const radians& pRadians) const
{
	return{ mRadians + pRadians };
}

radians radians::operator - (const radians& pRadians) const
{
	return{ mRadians - pRadians };
}

radians radians::operator * (const radians& pRadians) const
{
	return{ mRadians * pRadians };
}

radians radians::operator / (const radians& pRadians) const
{
	return{ mRadians / pRadians };
}

radians& radians::operator = (const radians& pRadians)
{
	mRadians = pRadians;
	return *this;
}

radians& radians::operator += (const radians& pRadians)
{
	mRadians += pRadians;
	return *this;
}

radians& radians::operator -= (const radians& pRadians)
{
	mRadians -= pRadians;
	return *this;
}

radians& radians::operator *= (const radians& pRadians)
{
	mRadians *= pRadians;
	return *this;
}

radians& radians::operator /= (const radians& pRadians)
{
	mRadians /= pRadians;
	return *this;
}

degrees::degrees()
{
	mDegrees = 0;
}

degrees::degrees(const float & pDegrees)
{
	mDegrees = pDegrees;
}

degrees::degrees(const degrees & pDegrees)
{
	mDegrees = pDegrees.mDegrees;
}

degrees::degrees(const radians & pRadians)
{
	mDegrees = pRadians.to_degrees().value();
}

radians degrees::to_radians() const
{
	return{ (mDegrees / 180.f) * math::pi };
}

float degrees::value() const
{
	return mDegrees;
}

degrees degrees::operator + (const degrees& pDegrees) const
{
	return{ mDegrees + pDegrees };
}

degrees degrees::operator - (const degrees& pDegrees) const
{
	return{ mDegrees - pDegrees };
}

degrees degrees::operator * (const degrees& pDegrees) const
{
	return{ mDegrees * pDegrees };
}

degrees degrees::operator / (const degrees& pDegrees) const
{
	return{ mDegrees / pDegrees };
}

degrees& degrees::operator = (const degrees& pDegrees)
{
	mDegrees = pDegrees;
	return *this;
}

degrees& degrees::operator += (const degrees& pDegrees)
{
	mDegrees += pDegrees;
	return *this;
}

degrees& degrees::operator -= (const degrees& pDegrees)
{
	mDegrees -= pDegrees;
	return *this;
}

degrees& degrees::operator *= (const degrees& pDegrees)
{
	mDegrees *= pDegrees;
	return *this;
}

degrees& degrees::operator /= (const degrees& pDegrees)
{
	mDegrees /= pDegrees;
	return *this;
}