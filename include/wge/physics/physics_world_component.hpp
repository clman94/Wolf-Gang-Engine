#pragma once

#include <queue>

#include <wge/core/component.hpp>
#include <wge/core/object_node.hpp>
#include <wge/math/vector.hpp>

#include <Box2D/Dynamics/b2WorldCallbacks.h>

class b2World;
class b2Body;
struct b2BodyDef;

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
class physics_world_component :
	public core::component
{
	WGE_COMPONENT("Physics World", 3)
public:

	physics_world_component(core::object_node* pObj);

	virtual ~physics_world_component();

	void set_gravity(math::vec2 pVec);
	math::vec2 get_gravity() const;

	b2Body* create_body(const b2BodyDef& pDef);
	void destroy_body(b2Body* pBody);

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

	void on_preupdate(float pDelta);
	void on_postupdate(float pDelta);

	void destroy_queued_bodies();

private:
	b2World* mWorld;
	contact_listener mContact_listener;
	std::queue<b2Body*> mBody_destruction_queue;

	friend class physics_component;
};

}