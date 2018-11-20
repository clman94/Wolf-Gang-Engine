#pragma once

#include <wge/math/aabb.hpp>
#include <wge/math/matrix.hpp>

namespace wge::math
{

inline mat44 ortho(float left, float right, float top, float bottom)
{
	mat44 result(1);
	result.m[0][0] = 2.f / (right - left);
	result.m[1][1] = 2.f / (top - bottom);
	result.m[3][0] = -(right + left) / (right - left);
	result.m[3][1] = -(top + bottom) / (top - bottom);
	return result;
}

inline mat44 ortho(const math::aabb& pAABB)
{
	return ortho(pAABB.min.x, pAABB.max.x, pAABB.min.y, pAABB.max.y);
}

} // namespace wge::math
