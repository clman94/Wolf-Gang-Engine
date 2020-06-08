#include <wge/physics/physics_world.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/physics/box_collider_component.hpp>
#include <wge/math/transform.hpp>

#include <wge/core/layer.hpp>

#include <Box2D/Box2D.h>

namespace wge::physics
{

physics_world::physics_world() :
	mWorld(std::make_unique<b2World>(b2Vec2(0, 1)))
{}

void physics_world::set_gravity(math::vec2 pVec)
{
	mWorld->SetGravity({ pVec.x, pVec.y });
}

math::vec2 physics_world::get_gravity() const
{
	b2Vec2 gravity = mWorld->GetGravity();
	return{ gravity.x, gravity.y };
}

b2World* physics_world::get_world() const
{
	return mWorld.get();
}

void physics_world::preupdate(core::layer& pLayer, float pDelta)
{
	// Create all the bodies
	for (auto [id, physics, transform] : pLayer.each<physics_component, math::transform>())
	{
		if (!physics.mBody)
		{
			b2BodyDef body_def;
			body_def.position.x = transform.position.x;
			body_def.position.y = transform.position.y;
			body_def.angle = transform.rotation;
			body_def.type = b2_dynamicBody;
			body_def.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(id));
			physics.mBody = mWorld->CreateBody(&body_def);
			physics.mBody->ResetMassData();
		}
	}

	// Create all the fixtures
	for (auto [id, collider, physics, transform] :
		pLayer.each<box_collider_component, physics_component, math::transform>())
	{
		if (!collider.mFixture && physics.mBody)
		{
			b2FixtureDef fixture_def;
			b2PolygonShape shape;
			collider.update_shape(transform.scale, &shape);
			fixture_def.shape = &shape;
			fixture_def.density = 1;
			fixture_def.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(id));
			collider.mFixture = physics.create_fixture(fixture_def);
			physics.mBody->ResetMassData();
		}
	}

	// Calculate physics
	mWorld->Step(pDelta, 1, 1);
	update_object_transforms(pLayer);
}

void physics_world::postupdate(core::layer& pLayer, float pDelta)
{
	// Update the body to the transform of the transform component
	for (auto [id, physics, transform] : pLayer.each<physics_component, math::transform>())
	{
		if (physics.mBody)
		{
			math::vec2 position = transform.position;
			math::radians rotation = transform.rotation;
			physics.mBody->SetTransform({ position.x, position.y }, rotation);
		}
	}

	// Update the shapes
	for (auto [id, collider, transform] : pLayer.each<box_collider_component, math::transform>())
	{
		// FIXME: Only update when the transform's scale actually changes.
		collider.update_current_shape(transform.scale);
	}
}

void physics_world::update_object_transforms(core::layer& pLayer)
{
	for (auto [id, physics, transform] : pLayer.each<physics_component, math::transform>())
	{
		if (physics.mBody)
		{
			b2Vec2 position = physics.mBody->GetPosition();
			transform.position = math::vec2{ position.x, position.y };
			transform.rotation = math::radians(physics.mBody->GetAngle());
		}
	}
}

} // namespacw wge::physics