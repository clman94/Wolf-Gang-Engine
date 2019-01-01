#include <wge/physics/physics_component.hpp>
#include <wge/physics/physics_world.hpp>
#include <wge/core/transform_component.hpp>
#include <wge/logging/log.hpp>

#include <Box2D/Box2D.h>

#include <iostream>

namespace wge::physics
{

physics_component::physics_component(core::component_id pId) :
	core::component(pId),
	mBody(nullptr),
	mType(type_rigidbody)
{
}

physics_component::~physics_component()
{
	if (mBody)
	{
		// Destroy body before the on_physics_reset event
		// otherwise mBody turns null.
		//mBody->GetWorld()->DestroyBody(mBody);

		// Because this body exists, there may still be fixtures connected
		// to it. Tell the colliders to clean up.
		//get_object()->send_down("on_physics_reset");
	}
}

json physics_component::on_serialize(core::serialize_type pType) const
{
	json result;

	if (pType & core::serialize_type::properties)
	{
		result["type"] = mType;
	}

	if (pType & core::serialize_type::runtime_state)
	{
		// b2Body not created yet? Cache it all for later when it is
		// created.
		if (!mBody_instance_cache.empty())
			result["runtime-state"] = mBody_instance_cache;
		else if (mBody)
			result["runtime-state"] = serialize_body();
	}

	return result;
}

void physics_component::on_deserialize(const json& pJson)
{
	set_type(pJson["type"]);
	if (pJson.find("runtime-state") != pJson.end())
		mBody_instance_cache = pJson["runtime-state"];
	else
		mBody_instance_cache.clear();
}

void physics_component::set_type(int pType)
{
	mType = pType;
	if (mBody)
		mBody->SetType(get_b2Body_type());
}

int physics_component::get_type() const
{
	return mType;
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

b2BodyType physics_component::get_b2Body_type() const
{
	switch (mType)
	{
	case type_rigidbody: return b2BodyType::b2_dynamicBody; break;
	case type_static: return b2BodyType::b2_staticBody; break;
	default: return b2BodyType::b2_staticBody;
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

} // namespace wge::physics