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
	subscribe_to(pObj, "on_physics_update_bodies", &physics_component::on_physics_update_bodies, this);
}

void physics_component::set_type(int pType)
{
	mType = pType;
	if (mBody)
		mBody->SetType(get_b2_type());
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
		auto transform = get_object()->get_component<core::transform_component>();
		assert(transform);
		b2BodyDef body_def;
		body_def.position.x = transform->get_absolute_position().x;
		body_def.position.y = transform->get_absolute_position().y;
		body_def.angle = transform->get_absolute_rotation();
		body_def.type = get_b2_type();
		body_def.userData = get_object();
		mBody = pComponent->create_body(body_def);
		get_object()->send_down("on_physics_update_colliders", this);
		mBody->ResetMassData();
		std::cout << "Body updated\n";
	}
}

void physics_component::on_preupdate(float)
{
	auto transform = get_object()->get_component<core::transform_component>();
	assert(transform);

	const b2Vec2& pos = mBody->GetPosition();
	transform->set_position(math::vec2(pos.x, pos.y));

	const float rot = mBody->GetAngle();
	transform->set_rotaton(rot);
}

b2BodyType physics_component::get_b2_type() const
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
