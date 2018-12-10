#include <wge/math/math.hpp>
#include <wge/math/vector.hpp>
#include <limits>

using namespace wge;
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

vec2& vec2::ceil_magnitude()
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

vec2 vec2::operator-() const
{
	return { -x, -y };
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

// This generates a lookup table for binomial coefficients at compile-time
// TODO: Test
template<std::size_t Tmax_rows>
class binomial_coeff_lookup_table
{
private:
	static constexpr std::size_t total_elements = Tmax_rows*Tmax_rows;

	template <typename T, std::size_t Tsize>
	class const_array
	{
	public:
		constexpr T& operator[](size_t i)
		{
			return values[i];
		}

		constexpr const T& operator[](size_t i) const
		{
			return values[i];
		}

	private:
		T values[Tsize];
	};

	typedef const_array<unsigned long, total_elements> lookup_table_t;

	static constexpr unsigned long const_binomial_coefficient(unsigned long n, unsigned long k)
	{
		k = math::min(k, n - k);

		unsigned long c = 1;
		for (unsigned long i = 1; i <= k; i++, n--)
		{
			// Seems to struggle at this part
			//if (c / i > UINT_MAX / n)
			//	return 0;
			c = c / i * n + c % i * n / i;
		}
		return c;
	}

	static constexpr lookup_table_t generate()
	{
		lookup_table_t result = {};
		for (std::size_t r = 0; r < Tmax_rows; r++)
			for (std::size_t c = 0; c < r * 2 + 1; c++)
				result[r * r + c] = const_binomial_coefficient(r + 1, c);
		return result;
	}

	static constexpr lookup_table_t table = generate();
public:
	static constexpr std::size_t rows = Tmax_rows;
	static constexpr unsigned long get(unsigned long n, unsigned long k)
	{
		return table[(n - 1) * (n - 1) + k];
	}
};

// Reference: https://en.wikipedia.org/wiki/Binomial_coefficient
unsigned long wge::math::binomial_coeff(unsigned long n, unsigned long k)
{
	typedef binomial_coeff_lookup_table<6> lookup_table;

	// Use lookup table
	if (lookup_table::rows <= n)
		return lookup_table::get(n, k);

	// Symmetry
	k = math::min(k, n - k);

	unsigned long c = 1;
	for (unsigned long i = 1; i <= k; i++, n--)
	{
		// Return 0 on overflow
		if (c / i > std::numeric_limits<unsigned long>::max() / n)
			return 0;
		c = c / i * n + c % i * n / i;
	}

	return c;
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

radians::radians(const float & pRadians) noexcept
{
	mRadians = pRadians;
}

radians::radians(const degrees & pDegrees) noexcept
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