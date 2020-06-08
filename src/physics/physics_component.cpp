#include <wge/physics/physics_component.hpp>
#include <wge/physics/physics_world.hpp>
#include <wge/logging/log.hpp>

#include <Box2D/Box2D.h>

#include <iostream>

namespace wge::physics
{

physics_component::physics_component() :
	mBody(nullptr)
{
}

void physics_component::set_linear_velocity(const math::vec2 & pVec)
{
	if (mBody)
		mBody->SetLinearVelocity({ pVec.x, pVec.y });
}

math::vec2 physics_component::get_linear_velocity() const
{
	const b2Vec2& vel = mBody->GetLinearVelocity();
	return{ vel.x, vel.y };
}

void physics_component::set_angular_velocity(math::radians pRads)
{
	if (mBody)
		mBody->SetAngularVelocity(pRads);
}

math::radians physics_component::get_angular_velocity() const
{
	return mBody->GetAngularVelocity();
}

void physics_component::apply_force(const math::vec2& pForce, const math::vec2& pPoint) const
{
	if (mBody)
		mBody->ApplyForce({ pForce.x, pForce.y }, { pPoint.x, pPoint.y }, true);
}

void physics_component::apply_force(const math::vec2& pForce) const
{
	if (mBody)
		mBody->ApplyForceToCenter({ pForce.x, pForce.y }, true);
}

void physics_component::set_fixed_rotation(bool pSet)
{
}

b2Fixture* physics_component::create_fixture(const b2FixtureDef & pDef)
{
	assert(mBody);
	return mBody->CreateFixture(&pDef);
}

} // namespace wge::physics