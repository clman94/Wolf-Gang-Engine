#include <wge/physics/box_collider_component.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/core/transform_component.hpp>

#include <Box2D/Box2D.h>

#include <iostream>

using namespace wge;
using namespace wge::physics;

box_collider_component::box_collider_component(core::object_node* pObj) :
	component(pObj)
{
	mFixture = nullptr;
	subscribe_to(pObj, "on_physics_update_colliders", &box_collider_component::on_physics_update_colliders, this);
	subscribe_to(pObj, "on_physics_reset", &box_collider_component::on_physics_reset, this);
	mSize = math::vec2(1, 1);
}

box_collider_component::~box_collider_component()
{
	if (mFixture)
	{
		assert(mPhysics_component);
		mPhysics_component->destroy_fixture(mFixture);
	}
}

void box_collider_component::set_size(const math::vec2 & pSize)
{
	if (pSize.x > 0 && pSize.y > 0)
	{
		mSize = pSize;
		if (mFixture)
		{
			update_shape(dynamic_cast<b2PolygonShape*>(mFixture->GetShape()));
			mFixture->GetBody()->ResetMassData();
		}
	}
}

math::vec2 box_collider_component::get_size() const
{
	return mSize;
}

void box_collider_component::update_shape(b2PolygonShape * pShape)
{
	assert(pShape);
	pShape->SetAsBox(mSize.x, mSize.y, b2Vec2(-mSize.x / 2, -mSize.y / 2), 0);
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
