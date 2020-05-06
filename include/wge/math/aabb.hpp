#pragma once

#include "math.hpp"
#include "vector.hpp"
#include "rect.hpp"

namespace wge::math
{

class aabb
{
public:
	union {
		struct {
			math::vec2 min, max;
		};
		float components[4];
	};

public:
	aabb() noexcept {}
	aabb(float pMin_x, float pMin_y, float pMax_x, float pMax_y) noexcept :
		min(pMin_x, pMin_y),
		max(pMax_x, pMax_y)
	{}
	aabb(const math::vec2& pMin, const math::vec2& pMax) noexcept :
		min(pMin),
		max(pMax)
	{}
	aabb(const rect& pRect) noexcept :
		min(pRect.position),
		max(pRect.position + pRect.size)
	{}
	aabb(const aabb& pAABB) noexcept :
		min(pAABB.min),
		max(pAABB.max)
	{}

	aabb& operator = (const aabb& pAABB) noexcept
	{
		min = pAABB.min;
		max = pAABB.max;
		return *this;
	}

	math::vec2 get_size() const noexcept
	{
		return max - min;
	}

	bool intersect(const math::vec2& pVec) const noexcept
	{
		return min.x <= pVec.x && min.y <= pVec.y
			&& max.x >= pVec.x && max.y >= pVec.y;
	}

	bool intersect(const math::aabb& pAABB) const noexcept
	{
		return min.x <= pAABB.max.x && min.y <= pAABB.max.y
			&& max.x >= pAABB.min.x && max.y >= pAABB.min.y;
	}

	void merge(const math::vec2& pVec) noexcept
	{
		min.x = math::min(min.x, pVec.x);
		min.y = math::min(min.y, pVec.y);
		max.x = math::max(max.x, pVec.x);
		max.y = math::max(max.y, pVec.y);
	}

	void merge(const math::aabb& pAABB) noexcept
	{
		min.x = math::min(min.x, pAABB.min.x);
		min.y = math::min(min.y, pAABB.min.y);
		max.x = math::max(max.x, pAABB.max.x);
		max.y = math::max(max.y, pAABB.max.y);
	}
};

} // namespace wge::math
