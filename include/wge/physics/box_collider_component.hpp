#pragma once

#include <wge/core/component.hpp>
#include <wge/core/object_node.hpp>
#include <wge/math/vector.hpp>

class b2World;
class b2Fixture;
class b2PolygonShape;

namespace wge::physics
{

class physics_component;

class box_collider_component :
	public core::component
{
	WGE_COMPONENT("Box Collider", 268);
public:
	box_collider_component(core::object_node* pObj);
	virtual ~box_collider_component();

	void set_size(const math::vec2& pSize);
	math::vec2 get_size() const;

private:
	void on_physics_update_colliders(physics_component* pComponent);
	void on_physics_reset();

	void update_shape(b2PolygonShape* pShape);

private:
	b2Fixture* mFixture;
	math::vec2 mSize;
	physics_component* mPhysics_component;
};

}