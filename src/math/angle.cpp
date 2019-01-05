#include <wge/math/math.hpp>
#include <wge/math/vector.hpp>
#include <limits>

namespace wge::math
{


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
