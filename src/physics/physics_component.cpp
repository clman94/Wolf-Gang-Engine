#include <wge/physics/physics_component.hpp>
#include <wge/physics/physics_world_component.hpp>
#include <wge/core/transform_component.hpp>
#include <wge/logging/log.hpp>

#include <Box2D/Box2D.h>

#include <iostream>

using namespace wge;
using namespace wge::physics;

physics_component::physics_component()
{
	mBody = nullptr;
}

physics_component::~physics_component()
{
	if (mBody)
	{
		// Destroy body before the on_physics_reset event
		// otherwise mBody turns null.
		mBody->GetWorld()->DestroyBody(mBody);

		// Because this body exists, there may still be fixtures connected
		// to it. Tell the colliders to clean up.
		//get_object()->send_down("on_physics_reset");
	}
}

json physics_component::serialize() const
{
	json result;
	result["type"] = mType;

	// deserialize() and then serialize() called in the same frame?
	// Just use the cached properties.
	if (!mBody_instance_cache.empty())
		result["body-instance"] = mBody_instance_cache;
	else if (mBody)
		result["body-instance"] = serialize_body();

	return result;
}

void physics_component::deserialize(const json& pJson)
{
	set_type(pJson["type"]);
	if (pJson.find("body-instance") != pJson.end())
		mBody_instance_cache = pJson["body-instance"];
	else
		mBody_instance_cache.clear();
}

void physics_component::set_type(int pType)
{
	mType = pType;
	if (mBody)
		mBody->SetType(get_b2Body_type());
}

void physics_component::set_linear_velocity(const math::vec2 & pVec)
{
	if (mBody)
		mBody->SetLinearVelocity({ pVec.x, pVec.y });
	else
	{
		// Create defaults
		serialize_and_cache_body();
		mBody_instance_cache["linear-velocity"] = pVec;
	}

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

b2Fixture* physics_component::create_fixture(const b2FixtureDef & pDef)
{
	assert(mBody);
	return mBody->CreateFixture(&pDef);
}

void physics_component::on_physics_update_bodies(physics_world_component * pComponent)
{
	if (!mBody)
	{
		//auto transform = get_object()->get_component<core::transform_component>();
		/*assert(transform);
		b2BodyDef body_def;
		body_def.position.x = transform->get_absolute_position().x;
		body_def.position.y = transform->get_absolute_position().y;
		body_def.angle = transform->get_absolute_rotation();
		body_def.type = get_b2Body_type();
		body_def.userData = get_object();
		mBody = pComponent->create_body(body_def);
		if (!mBody_instance_cache.empty())
			deserialize_body_from_cache(); // Finally deserialize if it was waiting for the body to be created
		mBody->ResetMassData();
		mPhysics_world = pComponent;
		log::debug() << WGE_LI << get_object()->get_name() << ": New Physics Body" << log::endm;*/
	}
	//get_object()->send_down("on_physics_update_colliders", this);
}

void physics_component::on_physics_reset()
{
	// Just clear everything.
	// The bodies and fixtures are expected to be cleaned
	// up by the world.
	mBody = nullptr;
	mPhysics_world = nullptr;
}

void physics_component::on_preupdate(float)
{
	// Update to the transform of the object to the body.
	// This is executed after physics have been calculated.
	update_object_transform();
}

void physics_component::on_postupdate(float)
{
	// Set the position of the body to the position of the transform.
	// This is done after the on_update event so if any scripts or the editor
	// modify the transform of the object, the body will update as well.
	update_body_transform();
}

void physics_component::on_parent_removed()
{
	if (mBody)
	{
		// Cache the body so when a new parent comes in, we can reload the same body
		mBody_instance_cache = serialize_body();
		mBody->GetWorld()->DestroyBody(mBody);
		mBody = nullptr;
		mPhysics_world = nullptr;

		// Tell all possible colliders that this body was removed and
		// they need to reset.
		//get_object()->send_down("on_physics_reset");
	}
}

b2BodyType physics_component::get_b2Body_type() const
{
	switch (mType)
	{
	case type_rigidbody: return b2BodyType::b2_dynamicBody; break;
	case type_static: return b2BodyType::b2_staticBody; break;
	default: return b2BodyType::b2_staticBody;
	}
}

void physics_component::update_object_transform()
{
	if (mBody)
	{
		/*auto transform = get_object()->get_component<core::transform_component>();
		assert(transform);

		const b2Vec2& pos = mBody->GetPosition();
		transform->set_position(math::vec2(pos.x, pos.y));

		const float rot = mBody->GetAngle();
		transform->set_rotaton(rot);*/
	}
}

void physics_component::update_body_transform()
{
	if (mBody)
	{
		/*auto transform = get_object()->get_component<core::transform_component>();
		assert(transform);
		b2Vec2 body_position{ transform->get_position().x, transform->get_position().y };
		float body_rotation{ transform->get_rotation() };
		mBody->SetTransform(body_position, body_rotation);*/
	}
}

json physics_component::serialize_body() const
{
	json result;
	if (mBody)
	{
		result["linear-velocity"] = mBody->GetLinearVelocity();
		result["linear-dampening"] = mBody->GetLinearDamping();
		result["angular-velocity"] = mBody->GetAngularVelocity();
		result["angular-dampening"] = mBody->GetAngularDamping();
	}
	else
	{
		// Create defaults
		result["linear-velocity"] = { 0.f, 0.f };
		result["linear-dampening"] = 0.f;
		result["angular-velocity"] = 0.f;
		result["angular-dampening"] = 0.f;
	}
	return result;
}

void physics_component::deserialize_body_from_cache()
{
	assert(mBody);
	if (!mBody_instance_cache.empty())
	{
		mBody->SetLinearVelocity(mBody_instance_cache["linear-velocity"]);
		mBody->SetLinearDamping(mBody_instance_cache["linear-dampening"]);
		mBody->SetAngularVelocity(mBody_instance_cache["angular-velocity"]);
		mBody->SetLinearDamping(mBody_instance_cache["angular-dampening"]);

		// We don't need this anymore
		mBody_instance_cache.clear();
	}
}

void physics_component::serialize_and_cache_body()
{
	if (mBody_instance_cache.empty())
		mBody_instance_cache = serialize_body();
}
