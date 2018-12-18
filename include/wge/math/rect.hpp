#pragma once

#include <wge/math/vector.hpp>

namespace wge::math
{

class aabb;

class rect
{
public:
	union {
		struct {
			vec2 position, size;
		};
		struct {
			float x, y, width, height;
		};
		float components[4];
	};

	rect() noexcept :
		position(0, 0), size(0, 0)
	{}
	rect(float pX, float pY, float pWidth, float pHeight) noexcept :
		position(pX, pY), size(pWidth, pHeight)
	{}
	rect(const math::vec2& pPosition, const math::vec2& pSize) noexcept :
		position(pPosition), size(pSize)
	{}
	rect(const aabb&) noexcept;
	rect(const rect& pRect) noexcept :
		position(pRect.position),
		size(pRect.size)
	{}

	rect& operator = (const rect& pRect) noexcept
	{
		position = pRect.position;
		size = pRect.size;
		return *this;
	}

	vec2 get_corner(unsigned int pIndex) const noexcept;

	bool intersects(const vec2& pVec) const noexcept;
};

} // namespace wge::math
