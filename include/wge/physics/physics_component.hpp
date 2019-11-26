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

class physics_component
{
public:
	enum {
		type_rigidbody,
		type_static
	};

	physics_component();
	virtual ~physics_component();

	void set_type(int pType);
	int get_type() const;

	void set_linear_velocity(const math::vec2& pVec);
	math::vec2 get_linear_velocity() const;

	void set_angular_velocity(math::radians pRads);
	math::radians get_angular_velocity() const;
	
	void apply_force(const math::vec2& pForce, const math::vec2& pPoint) const;
	void apply_force(const math::vec2& pForce) const;

	void set_fixed_rotation(bool pSet);

	b2Fixture* create_fixture(const b2FixtureDef& pDef);

private:
	// Get the box2d body type
	b2BodyType get_b2Body_type() const;

	json serialize_body() const;
	void deserialize_body_from_cache();
	void serialize_and_cache_body();

private:
	int mType;
	b2Body* mBody;

	// Since we can't create a body whenever we want to, we need store
	// the serialized data for when it is created.
	json mBody_instance_cache;

	friend class physics_world;
};

} // namespace wge::physics
