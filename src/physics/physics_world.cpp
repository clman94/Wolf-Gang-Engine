#include <wge/physics/physics_world.hpp>
#include <wge/physics/physics_component.hpp>

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
	// Check that all the physics bodies exist
	for (std::size_t i = 0; i < get_layer().get_object_count(); i++)
	{
		core::game_object obj = get_layer().get_object(i);
		core::transform_component* transform;
		physics_component* physics;
		get_layer().retrieve_components(obj, physics, transform);
		if (!physics->mBody)
		{
			b2BodyDef body_def;
			body_def.position.x = transform->get_absolute_position().x;
			body_def.position.y = transform->get_absolute_position().y;
			body_def.angle = transform->get_absolute_rotation();
			body_def.type = physics->get_b2Body_type();
			//body_def.userData = obj.get_instance_id().get_value();
			physics->mBody = mWorld->CreateBody(&body_def);
			physics->mBody->ResetMassData();
			log::debug() << WGE_LI << obj.get_name() << ": New Physics Body" << log::endm;
		}
	}

	// Calculate physics
	mWorld->Step(pDelta, 1, 1);
	update_object_transforms();
}

void physics_world::update_object_transforms()
{	
	// Check that all the physics bodies exist
	for (std::size_t i = 0; i < get_layer().get_object_count(); i++)
	{
		core::game_object obj = get_layer().get_object(i);
		core::transform_component* transform;
		physics_component* physics;
		get_layer().retrieve_components(obj, physics, transform);
		if (physics->mBody)
		{
			b2Vec2 position = physics->mBody->GetPosition();
			transform->set_position({ position.x, position.y });
			transform->set_rotaton(math::radians(physics->mBody->GetAngle()));
		}
	}
}

} // namespacw wge::physics