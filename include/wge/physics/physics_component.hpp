#pragma once

#include <wge/core/component.hpp>
#include <wge/core/object_node.hpp>
#include <wge/math/vector.hpp>

class b2Body;
class b2Fixture;
struct b2FixtureDef;
enum b2BodyType;


namespace wge::physics
{

class physics_world_component;

class physics_component :
	public core::component
{
	WGE_COMPONENT("Physics", 5431);
public:
	enum {
		type_rigidbody,
		type_static
	};

	physics_component(core::object_node* pObj);

	void set_type(int pType);

	b2Fixture* create_fixture(const b2FixtureDef& pDef);

private:
	void on_physics_update_bodies(physics_world_component* pComponent);

	void on_preupdate(float);

	b2BodyType get_b2_type() const;

private:
	std::vector<std::weak_ptr<core::object_node>> mObjects_with_collision;
	b2Body* mBody;

	int mType;
	friend class physics_world_component;
};

}