#include <wge/math/math.hpp>
#include <wge/math/vector.hpp>
#include <limits>

namespace wge::math
{

float vec2::magnitude() const noexcept
{
	return math::sqrt(x * x + y * y);
}

float vec2::distance(const vec2& pTo) const noexcept
{
	return (pTo - *this).magnitude();
}

vec2& vec2::rotate(const radians& pRadians) noexcept
{
	float s = math::sin(pRadians);
	float c = math::cos(pRadians);
	float tx = x, ty = y;
	x = tx * c - ty * s;
	y = tx * s + ty * c;
	return *this;
}

vec2& vec2::normalize() noexcept
{
	float mag = magnitude();
	if (mag != 0)
	{
		x /= mag;
		y /= mag;
	}
	return *this;
}

vec2& vec2::abs() noexcept
{
	x = math::abs(x);
	y = math::abs(y);
	return *this;
}

vec2& vec2::floor() noexcept
{
	x = math::floor(x);
	y = math::floor(y);
	return *this;
}

vec2& vec2::floor_magnitude() noexcept
{
	float mag = magnitude();
	*this *= math::floor(mag) / mag;
	return *this;
}

vec2& vec2::ceil() noexcept
{
	x = math::ceil(x);
	y = math::ceil(y);
	return *this;
}

vec2& vec2::ceil_magnitude() noexcept
{
	float mag = magnitude();
	*this *= math::ceil(mag) / mag;
	return *this;
}

vec2& vec2::round() noexcept
{
	x = math::round(x);
	y = math::round(y);
	return *this;
}

vec2& vec2::round_magnitude() noexcept
{
	float mag = magnitude();
	*this *= math::round(mag) / mag;
	return *this;
}

vec2 & vec2::swap_xy() noexcept
{
	std::swap(x, y);
	return *this;
}

vec2 vec2::operator + (const vec2& pR) const noexcept
{
	return { x + pR.x, y + pR.y };
}

vec2 vec2::operator - (const vec2& pR) const noexcept
{
	return { x - pR.x, y - pR.y };
}

vec2 vec2::operator * (const vec2& pR) const noexcept
{
	return { x * pR.x, y * pR.y };
}

vec2 vec2::operator / (const vec2& pR) const noexcept
{
	return { pR.x == 0 ? 0 : x / pR.x, pR.y == 0 ? 0 : y / pR.y };
}

vec2 vec2::operator - () const noexcept
{
	return { -x, -y };
}

vec2 vec2::operator * (const float& pR) const noexcept
{
	return { x * pR, y * pR };
}

vec2 vec2::operator / (const float& pR) const noexcept
{
	return { x / pR, y / pR };
}

vec2& vec2::operator = (const vec2& pR) noexcept
{
	x = pR.x;
	y = pR.y;
	return *this;
}

vec2& vec2::operator += (const vec2& pR) noexcept
{
	x += pR.x;
	y += pR.y;
	return *this;
}

vec2& vec2::operator -= (const vec2& pR) noexcept
{
	x -= pR.x;
	y -= pR.y;
	return *this;
}

vec2& vec2::operator *= (const vec2& pR) noexcept
{
	x *= pR.x;
	y *= pR.y;
	return *this;
}

vec2& vec2::operator /= (const vec2& pR) noexcept
{
	x /= pR.x;
	y /= pR.y;
	return *this;
}

vec2& vec2::operator *= (const float& pR) noexcept
{
	x *= pR;
	y *= pR;
	return *this;
}

vec2& vec2::operator /= (const float& pR) noexcept
{
	x /= pR;
	y /= pR;
	return *this;
}

bool vec2::operator==(const vec2 & pR) const noexcept
{
	return x == pR.x && y == pR.y;
}

bool vec2::operator!=(const vec2 & pR) const noexcept
{
	return !operator==(pR);
}

std::string vec2::to_string() const
{
	return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
}

degrees terminal_angle(const degrees& pDegrees) noexcept
{
	return pDegrees % deg_max;
}

radians terminal_angle(const radians& pRadians) noexcept
{
	return pRadians % rad_max;
}

degrees wge::math::radians_to_degrees(float pRadians) noexcept
{
	return{ radians{ pRadians } };
}

radians wge::math::degrees_to_radians(float pDegrees) noexcept
{
	return{ degrees{ pDegrees } };
}

radians::radians() noexcept
{
	mRadians = 0;
}

radians::radians(float pRadians) noexcept
{
	mRadians = pRadians;
}

radians::radians(const degrees& pDegrees) noexcept
{
	mRadians = pDegrees.to_radians().value();
}

degrees radians::to_degrees() const noexcept
{
	return{ (mRadians / math::pi) * math::deg_half };
}

float radians::value() const noexcept
{
	return mRadians;
}

radians radians::operator + (const radians& pRadians) const noexcept
{
	return{ mRadians + pRadians };
}

radians radians::operator - (const radians& pRadians) const noexcept
{
	return{ mRadians - pRadians };
}

radians radians::operator * (const radians& pRadians) const noexcept
{
	return{ mRadians * pRadians };
}

radians radians::operator / (const radians& pRadians) const noexcept
{
	return{ mRadians / pRadians };
}

radians radians::operator % (const radians& pRadians) const noexcept
{
	return math::positive_modulus(mRadians, pRadians.mRadians);
}

radians radians::operator - () const noexcept
{
	return radians(-mRadians);
}

radians& radians::operator = (const radians& pRadians) noexcept
{
	mRadians = pRadians;
	return *this;
}

radians& radians::operator += (const radians& pRadians) noexcept
{ 
	mRadians += pRadians;
	return *this;
}

radians& radians::operator -= (const radians& pRadians) noexcept
{
	mRadians -= pRadians;
	return *this;
}

radians& radians::operator *= (const radians& pRadians) noexcept
{
	mRadians *= pRadians;
	return *this;
}

radians& radians::operator /= (const radians& pRadians) noexcept
{
	mRadians /= pRadians;
	return *this;
}

bool radians::operator == (const radians& pRadians) const noexcept
{
	return mRadians == pRadians.mRadians;
}

bool radians::operator != (const radians& pRadians) const noexcept
{
	return mRadians != pRadians.mRadians;
}

bool radians::operator >= (const radians& pRadians) const noexcept
{
	return mRadians >= pRadians.mRadians;
}

bool radians::operator <= (const radians& pRadians) const noexcept
{
	return mRadians <= pRadians.mRadians;
}

bool radians::operator > (const radians& pRadians) const noexcept
{
	return mRadians > pRadians.mRadians;
}

bool radians::operator < (const radians& pRadians) const noexcept
{
	return mRadians < pRadians.mRadians;
}

degrees::degrees() noexcept
{
	mDegrees = 0;
}

degrees::degrees(float pDegrees) noexcept
{
	mDegrees = pDegrees;
}
degrees::degrees(const radians & pRadians) noexcept
{
	mDegrees = pRadians.to_degrees().value();
}

radians degrees::to_radians() const noexcept
{
	return{ (mDegrees / math::deg_half) * math::pi };
}

float degrees::value() const noexcept
{
	return mDegrees;
}

degrees degrees::operator + (const degrees& pDegrees) const noexcept
{
	return{ mDegrees + pDegrees };
}

degrees degrees::operator - (const degrees& pDegrees) const noexcept
{
	return{ mDegrees - pDegrees };
}

degrees degrees::operator * (const degrees& pDegrees) const noexcept
{
	return{ mDegrees * pDegrees };
}

degrees degrees::operator / (const degrees& pDegrees) const noexcept
{
	return{ mDegrees / pDegrees };
}

degrees degrees::operator%(const degrees & pDegrees) const noexcept
{
	return math::positive_modulus(mDegrees, pDegrees.mDegrees);
}

degrees degrees::operator - () const noexcept
{
	return degrees(-mDegrees);
}

degrees& degrees::operator = (const degrees& pDegrees) noexcept
{
	mDegrees = pDegrees;
	return *this;
}

degrees& degrees::operator += (const degrees& pDegrees) noexcept
{
	mDegrees += pDegrees;
	return *this;
}

degrees& degrees::operator -= (const degrees& pDegrees) noexcept
{
	mDegrees -= pDegrees;
	return *this;
}

degrees& degrees::operator *= (const degrees& pDegrees) noexcept
{
	mDegrees *= pDegrees;
	return *this;
}

degrees& degrees::operator /= (const degrees& pDegrees) noexcept
{
	mDegrees /= pDegrees;
	return *this;
}

bool degrees::operator == (const degrees & pDegrees) const noexcept
{
	return mDegrees == pDegrees.mDegrees;
}

bool degrees::operator != (const degrees & pDegrees) const noexcept
{
	return mDegrees != pDegrees.mDegrees;
}

bool degrees::operator >= (const degrees& pDegrees) const noexcept
{
	return mDegrees >= pDegrees.mDegrees;
}

bool degrees::operator <= (const degrees & pDegrees) const noexcept
{
	return mDegrees <= pDegrees.mDegrees;
}

bool degrees::operator > (const degrees & pDegrees) const noexcept
{
	return mDegrees > pDegrees.mDegrees;
}

bool degrees::operator < (const degrees & pDegrees) const noexcept
{
	return mDegrees < pDegrees.mDegrees;
}

} // namespace wge::math
