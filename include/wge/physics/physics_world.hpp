#pragma once

#include <queue>

#include <wge/core/component.hpp>
#include <wge/core/game_object.hpp>
#include <wge/math/vector.hpp>
#include <wge/core/system.hpp>

#include <Box2D/Box2D.h>

namespace wge::core
{
class layer;
}

namespace wge::physics
{


struct hit_info
{
	bool hit = false;
	core::object_id object_id;
	math::vec2 point;
	math::vec2 normal;
	float fraction = 0;
};

class physics_world
{
public:
	physics_world();

	void set_gravity(math::vec2 pVec);
	math::vec2 get_gravity() const;

	hit_info first_hit_raycast(const math::vec2& pA, const math::vec2& pB) const
	{
		struct callback : b2RayCastCallback
		{
			hit_info hit;
			virtual float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point,
				const b2Vec2& normal, float32 fraction) override
			{
				hit.hit = true;
				hit.object_id = static_cast<core::object_id>(reinterpret_cast<std::uintptr_t>(fixture->GetUserData()));
				hit.point = { point.x, point.y };
				hit.normal = { normal.x, normal.y };
				hit.fraction = fraction;
				return 0;
			}
		} mycallback;
		mWorld->RayCast(&mycallback, { pA.x, pA.y }, { pB.x, pB.y });
		return mycallback.hit;
	}
	
	template <typename Tcallable>
	void raycast(Tcallable&& pCallable, const math::vec2& pA, const math::vec2& pB) const
	{
		struct callback : b2RayCastCallback
		{
			Tcallable callable;
			hit_info hit;
			virtual float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point,
				const b2Vec2& normal, float32 fraction)
			{
				hit.hit = true;
				hit.object_id = static_cast<core::object_id>(reinterpret_cast<std::uintptr_t>(fixture->GetUserData()));
				hit.point = { point.x, point.y };
				hit.normal = { normal.x, normal.y };
				hit.fraction = fraction;
				callable(hit);
			}
		} mycallback{ pCallable };

		mWorld->RayCast(&mycallback, { pA.x, pA.y }, { pB.x, pB.y });
		return mycallback.hit;
	}

	b2World* get_world() const;

	void preupdate(core::layer& pLayer, float pDelta);
	void postupdate(core::layer& pLayer, float pDelta);
private:
	struct contact
	{
		enum {
			state_begin,
			state_continued,
			state_end
		};
		int state;
	};

	struct contact_listener :
		b2ContactListener
	{
		virtual void BeginContact(b2Contact* pContact)
		{
			/*b2Fixture* f1 = pContact->GetFixtureA();
			b2Fixture* f2 = pContact->GetFixtureB();
			b2Body* b1 = f1->GetBody();
			b2Body* b2 = f2->GetBody();
			if (physics_component* component =
				static_cast<physics_component*>(b1->GetUserData()))
				component->get_object()->send("on_collision_begin");
			if (physics_component* component =
				static_cast<physics_component*>(b2->GetUserData()))
				component->get_object()->send("on_collision_begin");*/
		}
	};

	void update_object_transforms(core::layer& pLayer);

private:
	// Unfortunately, box2d likes to have everything pointing
	// to eachother so we have to keep its world in heap.
	std::unique_ptr<b2World> mWorld;
	contact_listener mContact_listener;

	friend class physics_component;
};

} // namespace wge::physics
