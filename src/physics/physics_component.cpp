#include <wge/physics/physics_component.hpp>
#include <wge/physics/physics_world_component.hpp>
#include <wge/core/transform_component.hpp>

#include <Box2D/Box2D.h>

#include <iostream>

using namespace wge;
using namespace wge::physics;

physics_component::physics_component(core::object_node * pObj) :
	component(pObj)
{
	mBody = nullptr;
	mType = type_rigidbody;
	subscribe_to(pObj, "on_preupdate", &physics_component::on_preupdate, this);
	subscribe_to(pObj, "on_postupdate", &physics_component::on_postupdate, this);
	subscribe_to(pObj, "on_physics_update_bodies", &physics_component::on_physics_update_bodies, this);
	subscribe_to(pObj, "on_physics_reset", &physics_component::on_physics_reset, this);
}

physics_component::~physics_component()
{
	get_object()->send("on_physics_reset");
	mPhysics_world->destroy_body(mBody);
}

json physics_component::serialize() const
{
	json result;
	result["type"] = mType;
	return result;
}

void physics_component::deserialize(const json& pJson)
{
	set_type(pJson["type"]);
}

void physics_component::set_type(int pType)
{
	mType = pType;
	if (mBody)
		mBody->SetType(get_b2Body_type());
}

b2Fixture* physics_component::create_fixture(const b2FixtureDef & pDef)
{
	assert(mBody);
	return mBody->CreateFixture(&pDef);
}

void physics_component::destroy_fixture(b2Fixture * pFixture)
{
	mFixture_destruction_queue.push(pFixture);
}

void physics_component::on_physics_update_bodies(physics_world_component * pComponent)
{
	if (!mBody)
	{
		auto transform = get_object()->get_component<core::transform_component>();
		assert(transform);
		b2BodyDef body_def;
		body_def.position.x = transform->get_absolute_position().x;
		body_def.position.y = transform->get_absolute_position().y;
		body_def.angle = transform->get_absolute_rotation();
		body_def.type = get_b2Body_type();
		body_def.userData = get_object();
		mBody = pComponent->create_body(body_def);
		get_object()->send_down("on_physics_update_colliders", this);
		mBody->ResetMassData();
		std::cout << "Body updated\n";
	}
}

void physics_component::on_physics_reset()
{
	// Just clear everything.
	// The bodies and fixtures are expected to be cleaned
	// up by the world.
	mBody = nullptr;
	mPhysics_world = nullptr;
	std::queue<b2Fixture*>{}.swap(mFixture_destruction_queue);
}

void physics_component::on_preupdate(float)
{
	update_object_transform();
}

void physics_component::on_postupdate(float)
{
	destroy_queued_fixtures();

	// Set the position of the body to the position of the transform.
	// This is done after the on_update event so if any scripts or such
	// modify the position of the object, the body will update as well.
	update_body_transform();
}

b2BodyType physics_component::get_b2Body_type() const
{
	b2BodyType body_type;
	switch (mType)
	{
	case type_rigidbody: body_type = b2BodyType::b2_dynamicBody; break;
	case type_static: body_type = b2BodyType::b2_staticBody; break;
	default: body_type = b2BodyType::b2_staticBody;
	}
	return body_type;
}

void physics_component::destroy_queued_fixtures()
{
	// Has no body
	if (!mBody)
		std::queue<b2Fixture*>().swap(mFixture_destruction_queue); // Clear queue

	// Destroy all fixtures queued to be destroyed
	while (!mFixture_destruction_queue.empty())
	{
		mBody->DestroyFixture(mFixture_destruction_queue.front());
		mFixture_destruction_queue.pop();
	}
}

void physics_component::update_object_transform()
{
	if (mBody)
	{
		auto transform = get_object()->get_component<core::transform_component>();
		assert(transform);

		const b2Vec2& pos = mBody->GetPosition();
		transform->set_position(math::vec2(pos.x, pos.y));

		const float rot = mBody->GetAngle();
		transform->set_rotaton(rot);
	}
}

void physics_component::update_body_transform()
{
	if (mBody)
	{
		auto transform = get_object()->get_component<core::transform_component>();
		assert(transform);
		b2Vec2 body_position{ transform->get_position().x, transform->get_position().y };
		float body_rotation{ transform->get_rotation() };
		mBody->SetTransform(body_position, body_rotation);
	}
}
