#include <wge/physics/box_collider_component.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/core/transform_component.hpp>
#include <wge/logging/log.hpp>

#include <Box2D/Box2D.h>

#include <iostream>

using namespace wge;
using namespace wge::physics;

box_collider_component::box_collider_component(core::game_object* pObj) :
	component(pObj),
	mFixture(nullptr),
	mSize(1, 1),
	mIs_sensor(false)
{
	mFixture = nullptr;
	subscribe_to(pObj, "on_physics_update_colliders", &box_collider_component::on_physics_update_colliders, this);
	subscribe_to(pObj, "on_physics_reset", &box_collider_component::on_physics_reset, this);
	subscribe_to(pObj, "on_parent_removed", &box_collider_component::on_parent_removed, this);
	subscribe_to(pObj, "on_transform_changed", &box_collider_component::on_transform_changed, this);

	// Requirements
	require<core::transform_component>();
}

box_collider_component::~box_collider_component()
{
	if (mFixture)
		mFixture->GetBody()->DestroyFixture(mFixture);
}

json box_collider_component::serialize() const
{
	json result;
	result["offset"] = mOffset;
	result["size"] = mSize;
	result["rotation"] = mRotation;
	result["sensor"] = mIs_sensor;
	return result;
}

void box_collider_component::deserialize(const json & pJson)
{
	mOffset = pJson["offset"];
	mSize = pJson["size"];
	mRotation = pJson["rotation"];
	update_current_shape();
	set_sensor(pJson["sensor"]);
}

void box_collider_component::set_offset(const math::vec2 & pOffset)
{
	mOffset = pOffset;
	update_current_shape();
}

math::vec2 box_collider_component::get_offset() const
{
	return mOffset;
}

void box_collider_component::set_size(const math::vec2 & pSize)
{
	if (pSize.x > 0 && pSize.y > 0)
	{
		mSize = pSize;
		update_current_shape();
	}
}

math::vec2 box_collider_component::get_size() const
{
	return mSize;
}

void box_collider_component::set_rotation(math::radians pRads)
{
	mRotation = pRads;
	update_current_shape();
}

math::radians box_collider_component::get_rotation() const
{
	return mRotation;
}

void box_collider_component::set_sensor(bool pIs_sensor)
{
	mIs_sensor = pIs_sensor;
	if (mFixture)
		mFixture->SetSensor(pIs_sensor);
}

bool box_collider_component::is_sensor() const
{
	return mIs_sensor;
}

void box_collider_component::set_anchor(math::vec2 pRatio)
{
	mAnchor = pRatio;
	update_current_shape();
}

math::vec2 box_collider_component::get_anchor() const
{
	return mAnchor;
}

void box_collider_component::update_shape(b2PolygonShape * pShape)
{
	assert(pShape);

	math::vec2 skin_size(pShape->m_radius, pShape->m_radius);
	math::vec2 offset = mSize / 2 + mOffset + skin_size + mSize * mAnchor;
	math::vec2 h_size = (mSize - skin_size * 2) / 2;
	pShape->SetAsBox(h_size.x, h_size.y, b2Vec2(offset.x, offset.y), mRotation);
}

void box_collider_component::update_current_shape()
{
	if (mFixture)
	{
		update_shape(dynamic_cast<b2PolygonShape*>(mFixture->GetShape()));
		mFixture->GetBody()->ResetMassData();
	}
}

void box_collider_component::on_physics_update_colliders(physics_component * pComponent)
{
	if (!mFixture)
	{
		mPhysics_component = pComponent;

		b2FixtureDef fixture_def;

		b2PolygonShape shape;
		update_shape(&shape);
		fixture_def.shape = &shape;
		fixture_def.density = 2;
		fixture_def.isSensor = mIs_sensor;
		fixture_def.userData = get_object();

		mFixture = pComponent->create_fixture(fixture_def);

		log::debug() << WGE_LI << get_object()->get_name() << ": Collider Updated" << log::endm;
	}
}

void box_collider_component::on_physics_reset()
{
	// Clear everything.
	// Fixture is expected to be cleaned up my the body or world.
	mFixture = nullptr;
	mPhysics_component = nullptr;
}

void box_collider_component::on_parent_removed()
{
	log::debug() << WGE_LI << get_object()->get_name() << ": Box collider responding to removed parent" << log::endm;

	if (mFixture)
	{
		mFixture->GetBody()->DestroyFixture(mFixture);
		log::debug() << WGE_LI << get_object()->get_name() << ": Removed Fixture" << log::endm;
	}
	mFixture = nullptr;
	mPhysics_component = nullptr;
}

void box_collider_component::on_transform_changed()
{
	update_current_shape();
}
