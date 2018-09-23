#include <wge/physics/physics_world_component.hpp>

#include <Box2D/Box2D.h>

using namespace wge;
using namespace wge::physics;

physics_world_component::physics_world_component(core::object_node* pObj) :
	component(pObj)
{
	mWorld = new b2World(b2Vec2(0, 1));
	//mWorld->SetContactListener(&mContact_listener);
	subscribe_to(pObj, "on_preupdate", &physics_world_component::on_preupdate, this);
}

physics_world_component::~physics_world_component()
{
	get_object()->send_down("on_physics_reset");
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
	// Update the bodies
	get_object()->send_down("on_physics_update_bodies", this);

	// Calculate physics
	mWorld->Step(pDelta, 1, 1);
}
