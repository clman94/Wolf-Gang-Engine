#pragma once

#include <wge/math/math.hpp>
#include <wge/math/angle.hpp>
#include <wge/util/span.hpp>
#include <fmt/format.h>

#include <functional>
#include <type_traits>

namespace wge::math
{

// Placeholders for swizzling
static constexpr struct _x_t {} _x;
static constexpr struct _y_t {} _y;

namespace detail
{

template <typename Tvec, typename Tvalue>
struct vec2_swizzle_component
{
	static auto get(const Tvec& pVec, Tvalue& pValue)
	{
		return pValue;
	}
};

template <typename Tvec>
struct vec2_swizzle_component<Tvec, _x_t>
{
	static auto get(const Tvec& pVec, _x_t)
	{
		return pVec.components()[0];
	}
};

template <typename Tvec>
struct vec2_swizzle_component<Tvec, _y_t>
{
	static auto get(const Tvec& pVec, _y_t)
	{
		return pVec.components()[1];
	}
};

} // namespace detail

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

	template <typename = detail::only_floating_point<T>>
	basic_vec2& rotate(const radians& pRadians) noexcept
	{
		T s = math::sin(pRadians);
		T c = math::cos(pRadians);
		T tx = x, ty = y;
		x = tx * c - ty * s;
		y = tx * s + ty * c;
		return *this;
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2& normalize() noexcept
	{
		T mag = magnitude();
		if (mag != 0)
		{
			x /= mag;
			y /= mag;
		}
		return *this;
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2& abs() noexcept
	{
		x = math::abs(x);
		y = math::abs(y);
		return *this;
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2& floor() noexcept
	{
		x = math::floor(x);
		y = math::floor(y);
		return *this;
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2& floor_magnitude() noexcept
	{
		T mag = magnitude();
		*this *= math::floor(mag) / mag;
		return *this;
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2& ceil() noexcept
	{
		x = math::ceil(x);
		y = math::ceil(y);
		return *this;
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2& ceil_magnitude() noexcept
	{
		T mag = magnitude();
		*this *= math::ceil(mag) / mag;
		return *this;
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2& round() noexcept
	{
		x = math::round(x);
		y = math::round(y);
		return *this;
	}

	template <typename = detail::only_floating_point<T>>
	basic_vec2& round_magnitude() noexcept
	{
		T mag = magnitude();
		*this *= math::round(mag) / mag;
		return *this;
	}

	radians angle() const noexcept
	{
		float tx = static_cast<float>(x);
		float ty = static_cast<float>(y);
		return std::atan2(ty, tx);
	}

	constexpr basic_vec2& swap_xy() noexcept
	{
		std::swap(x, y);
		return *this;
	}

	constexpr basic_vec2& mirror_x() noexcept
	{
		x = -x;
		return *this;
	}

	constexpr basic_vec2& mirror_y() noexcept
	{
		y = -y;
		return *this;
	}

	constexpr basic_vec2& mirror_xy() noexcept
	{
		x = -x;
		y = -y;
		return *this;
	}
	
	constexpr bool is_zero() const noexcept
	{
		if constexpr (std::is_floating_point_v<T>)
			return almost_equal(x, 0.f) && almost_equal(y, 0.f);
		else
			return x == 0 && y == 0;
	}

	// Vector operations
	constexpr basic_vec2 operator + (const basic_vec2& pR) const noexcept { return{ x + pR.x, y + pR.y }; }
	constexpr basic_vec2 operator - (const basic_vec2& pR) const noexcept { return{ x - pR.x, y - pR.y }; }
	constexpr basic_vec2 operator * (const basic_vec2& pR) const noexcept { return{ x * pR.x, y * pR.y }; }
	constexpr basic_vec2 operator / (const basic_vec2& pR) const noexcept { return{ x / pR.x, y / pR.y }; }

	template <typename = detail::only_signed<T>>
	constexpr basic_vec2 operator - () const noexcept { return{ -x, -y }; }

	// Scalar operations
	constexpr basic_vec2 operator * (T pR) const noexcept { return{ x * pR, y * pR }; }
	constexpr basic_vec2 operator / (T pR) const noexcept { return{ x / pR, y / pR }; }

	// Vector assignments
	constexpr basic_vec2& operator = (const basic_vec2& ) noexcept = default;
	constexpr basic_vec2& operator += (const basic_vec2& pR) noexcept { return *this = *this + pR; };
	constexpr basic_vec2& operator -= (const basic_vec2& pR) noexcept { return *this = *this - pR; };
	constexpr basic_vec2& operator *= (const basic_vec2& pR) noexcept { return *this = *this * pR; };
	constexpr basic_vec2& operator /= (const basic_vec2& pR) noexcept { return *this = *this / pR; };

	// Scalar assignments
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

	template <typename Tx, typename Ty>
	constexpr basic_vec2 swizzle(Tx pX, Ty pY) const noexcept
	{
		return{
			static_cast<T>(detail::vec2_swizzle_component<basic_vec2, Tx>::get(*this, pX)),
			static_cast<T>(detail::vec2_swizzle_component<basic_vec2, Ty>::get(*this, pY))
		};
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
