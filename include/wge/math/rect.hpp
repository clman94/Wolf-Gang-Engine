#pragma once

#include <wge/math/vector.hpp>

namespace wge::math
{
class rect
{
public:
	// A very crude but convenient structure
	union {
		struct {
			union {
				vec2 position;
				struct {
					float x, y;
				};
			};
			union {
				vec2 size;
				struct {
					float width, height;
				};
			};
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

	rect& operator=(const rect&);

	vec2 get_corner(unsigned int pIndex) const;

	bool intersects(const vec2& pVec) const;
};

}