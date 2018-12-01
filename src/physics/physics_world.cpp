#include <wge/physics/physics_world.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/physics/box_collider_component.hpp>

#include <wge/core/transform_component.hpp>
#include <wge/core/layer.hpp>

#include <Box2D/Box2D.h>

namespace wge::physics
{

physics_world::physics_world(core::layer& pLayer) :
	core::system(pLayer),
	mWorld(std::make_unique<b2World>(b2Vec2(0, 1)))
{
	//mWorld->SetContactListener(&mContact_listener);
	// Physics are updated before game logic does (which happenes on the "update" event).
	//subscribe_to(pObj, "on_preupdate", &physics_world_component::on_preupdate, this);
}

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

void physics_world::preupdate(float pDelta)
{
	// Create all the bodies
	get_layer().for_each(
		[&](core::game_object pObj,
			physics_component& pPhysics,
			core::transform_component& pTransform)
	{
		if (!pPhysics.mBody)
		{
			b2BodyDef body_def;
			body_def.position.x = pTransform.get_absolute_position().x;
			body_def.position.y = pTransform.get_absolute_position().y;
			body_def.angle = pTransform.get_absolute_rotation();
			body_def.type = pPhysics.get_b2Body_type();
			//body_def.userData = obj.get_instance_id().get_value();
			pPhysics.mBody = mWorld->CreateBody(&body_def);
			pPhysics.mBody->ResetMassData();
			log::debug() << WGE_LI << pObj.get_name() << ": New Physics Body" << log::endm;
		}
	});

	// Create all the fixtures
	get_layer().for_each(
		[&](core::game_object pObj,
			box_collider_component& pCollider,
			physics_component& pPhysics)
	{
		if (!pCollider.mFixture && pPhysics.mBody)
		{
			b2FixtureDef fixture_def;
			b2PolygonShape shape;
			pCollider.update_shape(&shape);
			fixture_def.shape = &shape;
			fixture_def.density = 1;
			pCollider.mFixture = pPhysics.create_fixture(fixture_def);
			pPhysics.mBody->ResetMassData();
			log::debug() << WGE_LI << pObj.get_name() << ": New Collider" << log::endm;
		}
	});

	// Calculate physics
	mWorld->Step(pDelta, 1, 1);
	update_object_transforms();
}

void physics_world::postupdate(float pDelta)
{
	// Update the body to the transform of the transform component
	get_layer().for_each(
		[&](physics_component& pPhysics, core::transform_component& pTransform)
	{
		if (pPhysics.mBody)
		{
			math::vec2 position = pTransform.get_absolute_position();
			math::radians rotation = pTransform.get_rotation();
			pPhysics.mBody->SetTransform({ position.x, position.y }, rotation);
		}
	});
}

void physics_world::update_object_transforms()
{	
	get_layer().for_each(
		[&](physics_component& pPhysics, core::transform_component& pTransform)
	{
		if (pPhysics.mBody)
		{
			b2Vec2 position = pPhysics.mBody->GetPosition();
			pTransform.set_position({ position.x, position.y });
			pTransform.set_rotaton(math::radians(pPhysics.mBody->GetAngle()));
		}
	});
}

} // namespacw wge::physics