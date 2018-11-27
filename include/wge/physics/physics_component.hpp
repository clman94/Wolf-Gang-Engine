#pragma once

#include <queue>

#include <wge/core/component.hpp>
#include <wge/core/game_object.hpp>
#include <wge/math/vector.hpp>

class b2Body;
class b2Fixture;
struct b2FixtureDef;
enum b2BodyType;

namespace wge::physics
{

class physics_world;

class physics_component :
	public core::component
{
	WGE_COMPONENT("Physics", 5431);
public:
	enum {
		type_rigidbody,
		type_static
	};

	physics_component(core::component_id pId);
	virtual ~physics_component();

	virtual json serialize() const override;
	virtual void deserialize(const json& pJson) override;

	void set_type(int pType);

	void set_linear_velocity(const math::vec2& pVec);
	math::vec2 get_linear_velocity() const;

	void set_angular_velocity(math::radians pRads);
	math::radians get_angular_velocity() const;

	b2Fixture* create_fixture(const b2FixtureDef& pDef);

private:
	void on_physics_update_bodies(physics_world* pComponent);
	void on_physics_reset();
	void on_preupdate(float);
	void on_postupdate(float);
	void on_parent_removed();

	// Get the box2d body type
	b2BodyType get_b2Body_type() const;

	// Update the transform component to the
	// transform of the body.
	void update_object_transform();
	// Update the body transform with the transform of
	// the transform component.
	void update_body_transform();

	json serialize_body() const;
	void deserialize_body_from_cache();
	void serialize_and_cache_body();

private:
	int mType;
	b2Body* mBody;
	physics_world* mPhysics_world;

	// Since we can't create a body whenever we want to, we need store
	// the serialized data for when it is created.
	json mBody_instance_cache;

	friend class physics_world;
};

}