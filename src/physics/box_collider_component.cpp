#include <wge/physics/box_collider_component.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/core/transform_component.hpp>

#include <Box2D/Box2D.h>

#include <iostream>

using namespace wge;
using namespace wge::physics;

box_collider_component::box_collider_component(core::object_node* pObj) :
	component(pObj),
	mSize(1, 1)
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
	{
		assert(mPhysics_component);
		mPhysics_component->destroy_fixture(mFixture);
	}
}

json box_collider_component::serialize() const
{
	json result;
	result["offset"] = mOffset;
	result["size"] = mSize;
	result["rotation"] = mRotation;
	return result;
}

void box_collider_component::deserialize(const json & pJson)
{
	mOffset = pJson["offset"];
	mSize = pJson["size"];
	mRotation = pJson["rotation"];
	update_current_shape();
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

void box_collider_component::update_shape(b2PolygonShape * pShape)
{
	assert(pShape);

	math::vec2 offset = mSize / 2 + mOffset;
	pShape->SetAsBox(mSize.x / 2, mSize.y / 2, b2Vec2(offset.x, offset.y), mRotation);
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
		fixture_def.userData = get_object();

		mFixture = pComponent->create_fixture(fixture_def);

		std::cout << "Collider updated\n";
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
	if (mFixture)
	{
		mPhysics_component->destroy_fixture(mFixture);
		mFixture = nullptr;
		mPhysics_component = nullptr;
	}
}

void box_collider_component::on_transform_changed()
{
	update_current_shape();
}
