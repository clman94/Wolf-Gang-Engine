#include <wge/math/rect.hpp>
#include <wge/math/aabb.hpp>

namespace wge::math
{

rect::rect(const aabb& pAABB) noexcept :
	position(pAABB.min),
	size(pAABB.max - pAABB.min)
{}

vec2 rect::get_corner(unsigned int pIndex) const noexcept
{
	switch (pIndex % 4)
	{
	default:
	case 0: return position;
	case 1: return position + vec2(width, 0);
	case 2: return position + size;
	case 3: return position + vec2(0, height);
	}
}

bool rect::intersects(const vec2 & pVec) const noexcept
{
	return pVec.x >= x && pVec.x < x + width
		&& pVec.y >= y && pVec.y < y + height;
}

}