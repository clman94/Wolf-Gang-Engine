#include <wge/physics/physics_world_component.hpp>

#include <Box2D/Box2D.h>

namespace wge::physics
{

physics_world_component::physics_world_component()
{
	mWorld = new b2World(b2Vec2(0, 1));
	//mWorld->SetContactListener(&mContact_listener);
	// Physics are updated before game logic does (which happenes on the "update" event).
	//subscribe_to(pObj, "on_preupdate", &physics_world_component::on_preupdate, this);
}

physics_world_component::~physics_world_component()
{
	//get_object()->send_down("on_physics_reset");
	delete mWorld;
}

void physics_world_component::set_gravity(math::vec2 pVec)
{
	mWorld->SetGravity({ pVec.x, pVec.y });
}

math::vec2 physics_world_component::get_gravity() const
{
	b2Vec2 gravity = mWorld->GetGravity();
	return{ gravity.x, gravity.y };
}

b2Body* physics_world_component::create_body(const b2BodyDef & pDef)
{
	return mWorld->CreateBody(&pDef);
}

b2World* physics_world_component::get_world() const
{
	return mWorld;
}

void physics_world_component::on_preupdate(float pDelta)
{
	// Update the bodies.
	// This will only update the children of this node.
	// The reason for this limitation, is that the bodies can easily reference their
	// parent for the world node and also it doesn't make sense to have bodies
	// children of other bodies, it simply does not work that way.
	//get_object()->send("on_physics_update_bodies", this);

	// Calculate physics
	mWorld->Step(pDelta, 1, 1);
}

} // namespacw wge::physics