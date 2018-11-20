#pragma once

#include "math.hpp"
#include "vector.hpp"

namespace wge::math
{

struct aabb
{
	math::vec2 min, max;

	bool intersect(const math::vec2& pVec) const
	{
		return min.x <= pVec.x && min.y <= pVec.y
			&& max.x >= pVec.x && max.y >= pVec.y;
	}

	bool intersect(const math::aabb& pAABB) const
	{
		return min.x <= pAABB.max.x && min.y <= pAABB.max.y
			&& max.x >= pAABB.min.x && max.y >= pAABB.min.y;
	}

	void merge(const math::vec2& pVec)
	{
		min.x = math::min(min.x, pVec.x);
		min.y = math::min(min.y, pVec.y);
		max.x = math::max(max.x, pVec.x);
		max.y = math::max(max.y, pVec.y);
	}

	void merge(const math::aabb& pAABB)
	{
		min.x = math::min(min.x, pAABB.min.x);
		min.y = math::min(min.y, pAABB.min.y);
		max.x = math::max(max.x, pAABB.max.x);
		max.y = math::max(max.y, pAABB.max.y);
	}
};

} // namespace wge::math
