#pragma once

#include <wge/util/span.hpp>
#include <wge/math/math.hpp>

namespace wge::graphics
{

class color8;

// Templatize this?
class color
{
public:
	using type = float;

	type r, g, b, a;

	color() :
		r(0), g(0), b(0), a(1)
	{}
	color(const color& pColor) = default;
	color(type pR, type pG, type pB) :
		r(pR), g(pG), b(pB), a(1)
	{}
	color(type pR, type pG, type pB, type pA) :
		r(pR), g(pG), b(pB), a(pA)
	{}
	color(util::span<const type> pSpan)
	{
		assert(pSpan.size() >= 4);
		r = pSpan[0];
		g = pSpan[1];
		b = pSpan[2];
		a = pSpan[3];
	}
	color(const color8&) noexcept;

	auto components() noexcept
	{
		return util::span{ &r, 4 };
	}

	auto components() const noexcept
	{
		return util::span{ &r, 4 };
	}
};

class color8
{
public:
	using type = unsigned char;

	type r, g, b, a;

	color8() :
		r(0), g(0), b(0), a(255)
	{}
	color8(const color8&) = default;
	color8(type pR, type pG, type pB) :
		r(pR), g(pG), b(pB), a(1)
	{}
	color8(type pR, type pG, type pB, type pA) :
		r(pR), g(pG), b(pB), a(pA)
	{}
	color8(util::span<const type> pSpan) noexcept
	{
		assert(pSpan.size() >= 4);
		r = pSpan[0];
		g = pSpan[1];
		b = pSpan[2];
		a = pSpan[3];
	}
	color8(const color& pColor) :
		r(static_cast<type>(math::clamp(pColor.r, 0.f, 1.f) * 255.f)),
		g(static_cast<type>(math::clamp(pColor.g, 0.f, 1.f) * 255.f)),
		b(static_cast<type>(math::clamp(pColor.b, 0.f, 1.f) * 255.f)),
		a(static_cast<type>(math::clamp(pColor.a, 0.f, 1.f) * 255.f))
	{}

	auto components() noexcept
	{
		return util::span{ &r, 4 };
	}

	auto components() const noexcept
	{
		return util::span{ &r, 4 };
	}
};

inline color::color(const color8& pColor8) noexcept :
	r(static_cast<color::type>(pColor8.r) / 255.f),
	g(static_cast<color::type>(pColor8.g) / 255.f),
	b(static_cast<color::type>(pColor8.b) / 255.f),
	a(static_cast<color::type>(pColor8.a) / 255.f)
{}

}
