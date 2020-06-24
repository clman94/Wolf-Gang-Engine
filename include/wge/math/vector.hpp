#pragma once

#include <wge/math/math.hpp>
#include <wge/math/angle.hpp>
#include <wge/util/span.hpp>
#include <fmt/format.h>

#include <functional>
#include <type_traits>

namespace wge::math
{

template <typename T>
class basic_vec2
{
public:
	using element_type = T;
	T x, y;

public:
	constexpr basic_vec2() noexcept :
		x(0),
		y(0)
	{}
	constexpr basic_vec2(T pX, T pY) noexcept :
		x(pX),
		y(pY)
	{}
	template <typename Tconv, typename = std::enable_if_t<!std::is_same_v<T, Tconv>>>
	constexpr explicit basic_vec2(const basic_vec2<Tconv>& pVec) noexcept :
		x(static_cast<T>(pVec.x)),
		y(static_cast<T>(pVec.y))
	{}
	constexpr basic_vec2(const basic_vec2&) noexcept = default;

	constexpr auto components() noexcept
	{
		return util::span{ &x, 2 };
	}

	constexpr auto components() const noexcept
	{
		return util::span{ &x, 2 };
	}

	template <typename = detail::only_floating_point<T>>
	T magnitude() const noexcept
	{
		return math::sqrt(x * x + y * y);
	}

	template <typename = detail::only_floating_point<T>>
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
		return pNormal * (dot(pNormal) / math::pow<T>(pNormal.magnitude(), 2));
	}

	basic_vec2 reflect(const basic_vec2& pNormal) const noexcept
	{
		return *this - (project(pNormal) * 2);
	}

	template <typename = detail::only_floating_point<T>>
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

	template <typename = detail::only_floating_point<T>>
	basic_vec2 normalize() const noexcept
	{
		T mag = magnitude();
		if (mag == 0)
			return {};
		return { x / mag, y / mag };
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2 normal_to(const basic_vec2& pTo) const noexcept
	{
		return (pTo - *this).normalize();
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2 abs() const noexcept
	{
		return { math::abs(x), math::abs(y) };
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2 floor() const noexcept
	{
		return { math::floor(x), math::floor(y) };
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2 floor_magnitude() const noexcept
	{
		T mag = magnitude();
		if (mag == 0)
			return{};
		return *this * (math::floor(mag) / mag);
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2 ceil() const noexcept
	{
		return { math::ceil(x), math::ceil(y) };
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2 ceil_magnitude() const noexcept
	{
		T mag = magnitude();
		if (mag == 0)
			return{};
		return *this * (math::ceil(mag) / mag);
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2 round() const noexcept
	{
		return { math::round(x), math::round(y) };
	}

	template <typename = detail::only_floating_point<T>>
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

	void nan_to_zero() noexcept
	{
		if (std::isnan(x))
			x = 0;
		if (std::isnan(y))
			y = 0;
	}

	bool is_nan() const noexcept
	{
		return std::isnan(x) || std::isnan(y);
	}
	
	constexpr bool is_zero() const noexcept
	{
		if constexpr (std::is_floating_point_v<T>)
			return almost_equal(x, 0.f) && almost_equal(y, 0.f);
		else
			return x == 0 && y == 0;
	}

	constexpr basic_vec2& set(const basic_vec2& pTo) noexcept
	{
		return *this = pTo;
	}

	// Vector operations
	constexpr basic_vec2 operator + (const basic_vec2& pR) const noexcept { return{ x + pR.x, y + pR.y }; }
	constexpr basic_vec2 operator - (const basic_vec2& pR) const noexcept { return{ x - pR.x, y - pR.y }; }
	constexpr basic_vec2 operator * (const basic_vec2& pR) const noexcept { return{ x * pR.x, y * pR.y }; }
	constexpr basic_vec2 operator / (const basic_vec2& pR) const noexcept { return{ x / pR.x, y / pR.y }; }

	template <typename = detail::only_signed<T>>
	constexpr basic_vec2 operator - () const noexcept { return{ -x, -y }; }

	// Scalar operations
	constexpr basic_vec2 operator + (T pR) const noexcept { return{ x + pR, y + pR }; }
	constexpr basic_vec2 operator - (T pR) const noexcept { return{ x - pR, y - pR }; }
	constexpr basic_vec2 operator * (T pR) const noexcept { return{ x * pR, y * pR }; }
	constexpr basic_vec2 operator / (T pR) const noexcept { return{ x / pR, y / pR }; }

	// Vector assignments
	constexpr basic_vec2& operator = (const basic_vec2& ) noexcept = default;
	constexpr basic_vec2& operator += (const basic_vec2& pR) noexcept { return *this = *this + pR; };
	constexpr basic_vec2& operator -= (const basic_vec2& pR) noexcept { return *this = *this - pR; };
	constexpr basic_vec2& operator *= (const basic_vec2& pR) noexcept { return *this = *this * pR; };
	constexpr basic_vec2& operator /= (const basic_vec2& pR) noexcept { return *this = *this / pR; };

	// Scalar assignments
	constexpr basic_vec2& operator += (T pR) noexcept { return *this = *this + pR; };
	constexpr basic_vec2& operator -= (T pR) noexcept { return *this = *this - pR; };
	constexpr basic_vec2& operator *= (T pR) noexcept { return *this = *this * pR; };
	constexpr basic_vec2& operator /= (T pR) noexcept { return *this = *this / pR; };

	constexpr bool operator == (const basic_vec2& pR) const noexcept { return x == pR.x && y == pR.y; }
	constexpr bool operator != (const basic_vec2& pR) const noexcept { return !operator==(pR); }

	constexpr bool operator < (const basic_vec2& pR) const noexcept { return x < pR.x || (x == pR.x && y < pR.y); }
	constexpr bool operator <= (const basic_vec2& pR) const noexcept { return operator<(pR) || operator==(pR); }
	constexpr bool operator > (const basic_vec2& pR) const noexcept { return !operator<(pR) && !operator==(pR); }
	constexpr bool operator >= (const basic_vec2& pR) const noexcept { return !operator<(pR); }

	// Returns with format "({x}, {y})"
	std::string to_string() const
	{
		return fmt::format("({}, {})", x, y);
	}
};

template <typename Tx, typename Ty>
basic_vec2(Tx, Ty)->basic_vec2<std::common_type_t<Tx, Ty>>;

using fvec2 = basic_vec2<float>;
using dvec2 = basic_vec2<double>;
using ivec2 = basic_vec2<int>;
using lvec2 = basic_vec2<long>;
using vec2 = basic_vec2<float>; // Default vec2

template <typename T>
inline constexpr basic_vec2<T> operator + (T pVal, const math::basic_vec2<T>& pVec) noexcept
{
	return{ pVal + pVec.x , pVal + pVec.y };
}

template <typename T>
inline constexpr basic_vec2<T> operator - (T pVal, const math::basic_vec2<T>& pVec) noexcept
{
	return{ pVal - pVec.x , pVal - pVec.y };
}

template <typename T>
inline constexpr basic_vec2<T> operator * (T pVal, const math::basic_vec2<T>& pVec) noexcept
{
	return{ pVal * pVec.x , pVal * pVec.y };
}

template <typename T>
inline constexpr basic_vec2<T> operator / (T pVal, const math::basic_vec2<T>& pVec) noexcept
{
	return{ pVal / pVec.x , pVal / pVec.y };
}

template<typename T>
inline constexpr basic_vec2<T> swap_xy(const basic_vec2<T>& a) noexcept
{
	return basic_vec2<T>(a).swap_xy();
}

template<typename T>
inline constexpr T dot(const basic_vec2<T>& pA, const basic_vec2<T>& pB) noexcept
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
