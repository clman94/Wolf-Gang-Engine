#include <wge/math/rect.hpp>
using namespace wge;
using namespace wge::math;

rect::rect(const rect & pCopy)
{
	position = pCopy.position;
	size = pCopy.size;
}

rect& rect::operator=(const rect& pCopy)
{
	position = pCopy.position;
	size = pCopy.size;
	return *this;
}

vec2 rect::get_corner(unsigned int pIndex) const
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

bool rect::intersects(const vec2 & pVec) const
{
	return pVec.x >= x && pVec.x < x + width
		&& pVec.y >= y && pVec.y < y + height;
}
