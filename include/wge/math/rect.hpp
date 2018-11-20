#pragma once

#include <wge/math/vector.hpp>

namespace wge::math
{
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

	rect() :
		position(0, 0), size(0, 0)
	{}

	rect(const rect&);

	rect(float pX, float pY, float pWidth, float pHeight) :
		position(pX, pY), size(pWidth, pHeight)
	{}

	rect(const math::vec2& pPosition, const math::vec2& pSize) :
		position(pPosition), size(pSize)
	{}

	rect& operator=(const rect&);

	vec2 get_corner(unsigned int pIndex) const;

	bool intersects(const vec2& pVec) const;
};

}