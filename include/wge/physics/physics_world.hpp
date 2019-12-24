#pragma once

#include <queue>

#include <wge/core/component.hpp>
#include <wge/core/game_object.hpp>
#include <wge/math/vector.hpp>
#include <wge/core/system.hpp>

#include <Box2D/Dynamics/b2WorldCallbacks.h>

class b2World;
class b2Body;
struct b2BodyDef;

namespace wge::core
{
class layer;
}

namespace wge::physics
{


/*

Events sent:
	void on_physics_update_bodies(physics_world_component*);
	  - Used by physics_component's to update their bodies to
		the current world.

Events recieved:
	void on_preupdate(float);
*/
class physics_world :
	public core::system
{
	WGE_SYSTEM("Physics World", 3)
public:
	physics_world();

	void set_gravity(math::vec2 pVec);
	math::vec2 get_gravity() const;

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
