#pragma once

#include <queue>

#include <wge/core/component.hpp>
#include <wge/core/game_object.hpp>
#include <wge/math/vector.hpp>
#include <wge/core/system.hpp>
#include <wge/math/aabb.hpp>

#include <Box2D/Box2D.h>

namespace wge::core
{
class layer;
}

namespace wge::physics
{


struct raycast_hit_info
{
	bool hit = false;
	core::object_id object_id;
	math::vec2 point;
	math::vec2 normal;

	operator bool() const noexcept
	{
		return hit;
	}
};

class physics_world
{
public:
	physics_world();

	void set_gravity(math::vec2 pVec);
	math::vec2 get_gravity() const;

	void imgui_debug(float delta);
	void set_raycast_debug_enabled(bool pEnabled)
	{
		mRaycast_debug_enabled = pEnabled;
	}
	bool get_raycast_debug_enabled() const noexcept
	{
		return mRaycast_debug_enabled;
	}
	void set_collision_debug_enabled(bool pEnabled)
	{
		mCollision_debug_enable = pEnabled;
	}

	bool get_collision_debug_enabled() const noexcept
	{
		return mCollision_debug_enable;
	}

	raycast_hit_info raycast_closest(const math::vec2& pA, const math::vec2& pB) const
	{
		// Box2d doesn't know how to handle rays with zero length
		// so let's just return early.
		if (pA == pB)
			return raycast_hit_info{};
		struct callback : b2RayCastCallback
		{
			raycast_hit_info hit;
			virtual float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point,
				const b2Vec2& normal, float32 fraction) override
			{
				hit.hit = true;
				hit.object_id = static_cast<core::object_id>(reinterpret_cast<std::uintptr_t>(fixture->GetUserData()));
				hit.point = { point.x, point.y };
				hit.normal = { normal.x, normal.y };
				return fraction;
			}
		} mycallback;
		mWorld->RayCast(&mycallback, { pA.x, pA.y }, { pB.x, pB.y });

		// Debug raycast.
		if (mRaycast_debug_enabled && mRaycast_debugs.size() < 500){
			raycast_debug db;
			db.from = pA;
			db.to = pB;
			db.hit = mycallback.hit.hit;
			db.point = mycallback.hit.point;
			mRaycast_debugs.push_back(db);
		}

		return mycallback.hit;
	}
	
	template <typename Tcallable>
	void raycast(Tcallable&& pCallable, const math::vec2& pA, const math::vec2& pB) const
	{
		struct callback : b2RayCastCallback
		{
			Tcallable callable;
			virtual float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point,
				const b2Vec2& normal, float32 fraction) override
			{
				raycast_hit_info hit;
				hit.hit = true;
				hit.object_id = static_cast<core::object_id>(reinterpret_cast<std::uintptr_t>(fixture->GetUserData()));
				hit.point = { point.x, point.y };
				hit.normal = { normal.x, normal.y };
				if (callable(hit))
					return 1;
				return 0;
			}

			callback(Tcallable&& pCallable) :
				callable(std::forward<Tcallable>(pCallable))
			{}
		};
		callback mycallback{ std::forward<Tcallable>(pCallable) };

		mWorld->RayCast(&mycallback, { pA.x, pA.y }, { pB.x, pB.y });
	}

	bool test_aabb(const math::aabb& pAabb) const
	{
		// Callback that stops on the first hit.
		struct callback : b2QueryCallback
		{
			bool hit = false;
			virtual bool ReportFixture(b2Fixture* fixture) override
			{
				hit = true;
				return false;
			}
		} mycallback;

		// Create a box2d compatible structure.
		b2AABB aabb;
		aabb.lowerBound.x = pAabb.min.x;
		aabb.lowerBound.y = pAabb.min.y;
		aabb.upperBound.x = pAabb.max.x;
		aabb.upperBound.y = pAabb.max.y;
		mWorld->QueryAABB(&mycallback, aabb);

		return mycallback.hit;
	}

	void clear_all()
	{
		b2Body* i = mWorld->GetBodyList();
		while (i)
		{
			b2Body* temp = i;
			i = i->GetNext();
			mWorld->DestroyBody(temp);
		}
	}

	b2World* get_world() const;

	void preupdate(core::layer& pLayer, float pDelta);
	void postupdate(core::layer& pLayer, float pDelta);
private:
	void update_object_transforms(core::layer& pLayer);

	struct raycast_debug
	{
		math::vec2 from, to;
		math::vec2 point;
		bool hit = false;
		float timer = 1;
	};
	bool mCollision_debug_enable = false;
	bool mRaycast_debug_enabled = false;
	mutable std::vector<raycast_debug> mRaycast_debugs;

private:
	// Unfortunately, box2d likes to have everything pointing
	// to eachother so we have to keep its world in heap.
	std::unique_ptr<b2World> mWorld;

	friend class physics_component;
};

} // namespace wge::physics
