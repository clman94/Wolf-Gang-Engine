#pragma once

#include <wge/math/math.hpp>
#include <wge/math/angle.hpp>
#include <wge/util/span.hpp>
#include <fmt/format.h>

#include <functional>
#include <type_traits>

namespace wge::math
{

namespace detail
{

template <
	typename basic_vec2,
	template <typename> typename TTbasic_vec2,
	typename T>
struct common_vec2
{
	using element_type = T;
	T x, y;

	constexpr common_vec2() noexcept :
		x(0),
		y(0)
	{}
	constexpr common_vec2(T pX, T pY) noexcept :
		x(pX),
		y(pY)
	{}
	template <typename Tconv, typename = std::enable_if_t<!std::is_same_v<T, Tconv>>>
	constexpr explicit common_vec2(const TTbasic_vec2<Tconv>& pVec) noexcept :
		x(static_cast<T>(pVec.x)),
		y(static_cast<T>(pVec.y))
	{}
	constexpr common_vec2(const common_vec2&) noexcept = default;
	constexpr common_vec2(common_vec2&&) noexcept = default;
	constexpr common_vec2& operator=(const common_vec2&) noexcept = default;
	constexpr common_vec2& operator=(common_vec2&&) noexcept = default;

	constexpr auto components() noexcept
	{
		return util::span{ &x, 2 };
	}

	constexpr auto components() const noexcept
	{
		return util::span{ &x, 2 };
	}

	constexpr basic_vec2 swap_xy() const noexcept
	{
		return { y, x };
	}

	constexpr basic_vec2 mirror_x() const noexcept
	{
		return { -x, y };
	}

	constexpr basic_vec2 mirror_y() const noexcept
	{
		return { x, -y };
	}

	constexpr basic_vec2 mirror_xy() const noexcept
	{
		return { -x, -y };
	}

	constexpr bool is_zero() const noexcept
	{
		return x == 0 && y == 0;
	}

	constexpr basic_vec2& set(const basic_vec2& pTo) noexcept
	{
		return static_cast<basic_vec2&>(*this) = pTo;
	}

	// Returns with format "({x}, {y})"
	std::string to_string() const
	{
		return fmt::format("({}, {})", x, y);
	}
};

} // namespace detail

template <typename T, bool = std::is_floating_point<T>::value>
class basic_vec2;

template <typename T>
class basic_vec2<T, true> :
	public detail::common_vec2<basic_vec2<T, true>, basic_vec2, T>
{
public:
	using common_vec2::common_vec2;

	T magnitude() const noexcept
	{
		return math::sqrt(x * x + y * y);
	}

	T distance(const basic_vec2& pTo) const noexcept
	{
		return (pTo - *this).magnitude();
	}

	T dot(const basic_vec2& pOther) const noexcept
	{
		return x * pOther.x + y * pOther.y;
	}

	basic_vec2 project(const basic_vec2& pNormal) const noexcept
	{
		return pNormal * (dot(pNormal) / math::pow<T>(pNormal.magnitude(), static_cast<T>(2)));
	}

	basic_vec2 reflect(const basic_vec2& pNormal) const noexcept
	{
		return *this - (project(pNormal) * static_cast<T>(2));
	}

	basic_vec2 rotate(const radians& pRadians) const noexcept
	{
		T s = math::sin(pRadians);
		T c = math::cos(pRadians);
		return { x * c - y * s, x * s + y * c };
	}

	basic_vec2 rotate_around(const radians& pRadians, const basic_vec2& pOrigin) const noexcept
	{
		return (*this - pOrigin).rotate(pRadians) + pOrigin;
	}

	basic_vec2 normalize() const noexcept
	{
		T mag = magnitude();
		if (mag == 0)
			return {};
		return { x / mag, y / mag };
	}

	basic_vec2 normal_to(const basic_vec2& pTo) const noexcept
	{
		return (pTo - *this).normalize();
	}

	basic_vec2 abs() const noexcept
	{
		return { math::abs(x), math::abs(y) };
	}

	basic_vec2 floor() const noexcept
	{
		return { math::floor(x), math::floor(y) };
	}

	basic_vec2 floor_magnitude() const noexcept
	{
		T mag = magnitude();
		if (mag == 0)
			return{};
		return *this * (math::floor(mag) / mag);
	}

	basic_vec2 ceil() const noexcept
	{
		return { math::ceil(x), math::ceil(y) };
	}

	basic_vec2 ceil_magnitude() const noexcept
	{
		T mag = magnitude();
		if (mag == 0)
			return{};
		return *this * (math::ceil(mag) / mag);
	}

	basic_vec2 round() const noexcept
	{
		return { math::round(x), math::round(y) };
	}

	basic_vec2 round_magnitude() const noexcept
	{
		T mag = magnitude();
		if (mag == 0)
			return{};
		return *this * (math::round(mag) / mag);
	}

	radians angle() const noexcept
	{
		float tx = static_cast<float>(x);
		float ty = static_cast<float>(y);
		return std::atan2(ty, tx);
	}

	radians angle_to(const basic_vec2& pOther) const noexcept
	{
		return (pOther - *this).angle();
	}

	constexpr basic_vec2 nan_to_zero() const noexcept
	{
		return { (x == x) ? x : 0, (y == y) ? y : 0 };
	}

	constexpr bool is_nan() const noexcept
	{
		return x != x || y != y;
	}
};

template <typename T>
class basic_vec2<T, false> :
	public detail::common_vec2<basic_vec2<T, false>, basic_vec2, T>
{
public:
	using common_vec2::common_vec2;
};

// Vector operations
template <typename T> constexpr basic_vec2<T> operator + (const basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return{ pVec.x + pR.x, pVec.y + pR.y }; }
template <typename T> constexpr basic_vec2<T> operator - (const basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return{ pVec.x - pR.x, pVec.y - pR.y }; }
template <typename T> constexpr basic_vec2<T> operator * (const basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return{ pVec.x * pR.x, pVec.y * pR.y }; }
template <typename T> constexpr basic_vec2<T> operator / (const basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return{ pVec.x / pR.x, pVec.y / pR.y }; }

// Scalar operations
template <typename T> constexpr basic_vec2<T> operator + (const basic_vec2<T>& pVec, T pR) noexcept { return{ pVec.x + pR, pVec.y + pR }; }
template <typename T> constexpr basic_vec2<T> operator - (const basic_vec2<T>& pVec, T pR) noexcept { return{ pVec.x - pR, pVec.y - pR }; }
template <typename T> constexpr basic_vec2<T> operator * (const basic_vec2<T>& pVec, T pR) noexcept { return{ pVec.x * pR, pVec.y * pR }; }
template <typename T> constexpr basic_vec2<T> operator / (const basic_vec2<T>& pVec, T pR) noexcept { return{ pVec.x / pR, pVec.y / pR }; }

// Vector assignments
template <typename T> constexpr basic_vec2<T>& operator += (basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return pVec = pVec + pR; };
template <typename T> constexpr basic_vec2<T>& operator -= (basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return pVec = pVec - pR; };
template <typename T> constexpr basic_vec2<T>& operator *= (basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return pVec = pVec * pR; };
template <typename T> constexpr basic_vec2<T>& operator /= (basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return pVec = pVec / pR; };

// Scalar assignments
template <typename T> constexpr basic_vec2<T>& operator += (basic_vec2<T>& pVec, T pR) noexcept { return pVec = pVec + pR; };
template <typename T> constexpr basic_vec2<T>& operator -= (basic_vec2<T>& pVec, T pR) noexcept { return pVec = pVec - pR; };
template <typename T> constexpr basic_vec2<T>& operator *= (basic_vec2<T>& pVec, T pR) noexcept { return pVec = pVec * pR; };
template <typename T> constexpr basic_vec2<T>& operator /= (basic_vec2<T>& pVec, T pR) noexcept { return pVec = pVec / pR; };

// Comparing
template <typename T> constexpr bool operator == (const basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return pVec.x == pR.x && pVec.y == pR.y; }
template <typename T> constexpr bool operator != (const basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return !operator==(pVec, pR); }
template <typename T> constexpr bool operator <  (const basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return pVec.x < pR.x || (pVec.x == pR.x && pVec.y < pR.y); }
template <typename T> constexpr bool operator <= (const basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return operator<(pVec, pR) || operator==(pVec, pR); }
template <typename T> constexpr bool operator >  (const basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return !operator<(pVec, pR) && !operator==(pVec, pR); }
template <typename T> constexpr bool operator >= (const basic_vec2<T>& pVec, const basic_vec2<T>& pR) noexcept { return !operator<(pVec, pR); }

// Inverse scalar operations
template <typename T> constexpr basic_vec2<T> operator + (T pVal, const math::basic_vec2<T>& pVec) noexcept { return{ pVal + pVec.x , pVal + pVec.y }; }
template <typename T> constexpr basic_vec2<T> operator - (T pVal, const math::basic_vec2<T>& pVec) noexcept { return{ pVal - pVec.x , pVal - pVec.y }; }
template <typename T> constexpr basic_vec2<T> operator * (T pVal, const math::basic_vec2<T>& pVec) noexcept { return{ pVal * pVec.x , pVal * pVec.y }; }
template <typename T> constexpr basic_vec2<T> operator / (T pVal, const math::basic_vec2<T>& pVec) noexcept { return{ pVal / pVec.x , pVal / pVec.y }; }

template <typename T> constexpr basic_vec2<T> operator - (const basic_vec2<T>& pVec) noexcept { return{ -pVec.x, -pVec.y }; }

using fvec2 = basic_vec2<float>;
using dvec2 = basic_vec2<double>;
using ivec2 = basic_vec2<int>;
using lvec2 = basic_vec2<long>;
using vec2 = basic_vec2<float>; // Default vec2

template<typename T>
constexpr basic_vec2<T> swap_xy(const basic_vec2<T>& a) noexcept
{
	return basic_vec2<T>(a).swap_xy();
}

template<typename T>
constexpr T dot(const basic_vec2<T>& pA, const basic_vec2<T>& pB) noexcept
{
	return pA.x * pB.x + pA.y * pB.y;
}

template<typename T>
inline T magnitude(const basic_vec2<T>& pA) noexcept
{
	return pA.magnitude();
}

template<typename T>
inline T distance(const basic_vec2<T>& pA, const basic_vec2<T>& pB) noexcept
{
	return pA.distance(pB);
}

template<typename T>
inline basic_vec2<T> clamp_components(const basic_vec2<T>& pA, const basic_vec2<T>& pMin, const basic_vec2<T>& pMax) noexcept
{
	return { math::clamp(pA.x, pMin.x, pMax.x), math::clamp(pA.y, pMin.y, pMax.y) };
}

} // namespace wge::math

namespace std
{

template<typename T>
struct hash<wge::math::basic_vec2<T>>
{
	using vec = wge::math::basic_vec2<T>;
	std::size_t operator()(vec const& pVec) const noexcept
	{
		std::size_t x = std::hash<T>{}(pVec.x);
		std::size_t y = std::hash<T>{}(pVec.y);
		return x ^ (y << 1);
	}
};

} // namespace std
