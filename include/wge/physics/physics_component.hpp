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

class sprite_fixture
{
private:
	b2Fixture* mFixture;
	friend class physics_world;
};

class physics_component
{
public:
	enum type {
		dynamic_physics,
		static_physics
	};

	physics_component();

	void set_linear_velocity(const math::vec2& pVec);
	math::vec2 get_linear_velocity() const;

	void set_angular_velocity(math::radians pRads);
	math::radians get_angular_velocity() const;
	
	void apply_force(const math::vec2& pForce, const math::vec2& pPoint) const;
	void apply_force(const math::vec2& pForce) const;

	void set_fixed_rotation(bool pSet);

private:
	b2Body* mBody;
	friend class physics_world;
};

} // namespace wge::physics
