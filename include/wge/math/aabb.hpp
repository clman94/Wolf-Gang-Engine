#pragma once

#include "math.hpp"
#include "vector.hpp"
#include "rect.hpp"

namespace wge::math
{

class aabb
{
public:
	math::vec2 min, max;

public:
	constexpr aabb() noexcept {}
	constexpr aabb(float pMin_x, float pMin_y, float pMax_x, float pMax_y) noexcept :
		min(pMin_x, pMin_y),
		max(pMax_x, pMax_y)
	{}
	constexpr aabb(const math::vec2& pMin, const math::vec2& pMax) noexcept :
		min(pMin),
		max(pMax)
	{}
	constexpr aabb(const rect& pRect) noexcept :
		min(pRect.position),
		max(pRect.position + pRect.size)
	{}

	constexpr math::vec2 point(std::size_t pIndex) const noexcept
	{
		switch (pIndex % 4)
		{
		case 0: return min;
		case 1: return math::vec2(max.x, min.y);
		case 2: return max;
		case 3: return math::vec2(min.x, max.y);
		default: return min; // Should not happen.
		}
	}

	constexpr bool intersect(const math::vec2& pVec) const noexcept
	{
		return min.x <= pVec.x && min.y <= pVec.y
			&& max.x >= pVec.x && max.y >= pVec.y;
	}

	constexpr bool intersect(const math::aabb& pAABB) const noexcept
	{
		return min.x <= pAABB.max.x && min.y <= pAABB.max.y
			&& max.x >= pAABB.min.x && max.y >= pAABB.min.y;
	}

	constexpr float x_diff() const noexcept
	{
		return max.x - min.x;
	}

	constexpr float y_diff() const noexcept
	{
		return max.y - min.y;
	}

	constexpr void merge(const math::vec2& pVec) noexcept
	{
		min.x = math::min(min.x, pVec.x);
		min.y = math::min(min.y, pVec.y);
		max.x = math::max(max.x, pVec.x);
		max.y = math::max(max.y, pVec.y);
	}

	constexpr void merge(const math::aabb& pAABB) noexcept
	{
		min.x = math::min(min.x, pAABB.min.x);
		min.y = math::min(min.y, pAABB.min.y);
		max.x = math::max(max.x, pAABB.max.x);
		max.y = math::max(max.y, pAABB.max.y);
	}
};

} // namespace wge::math
