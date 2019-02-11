#pragma once

#include <wge/core/component.hpp>
#include <wge/core/game_object.hpp>
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
	box_collider_component();
	virtual ~box_collider_component();

	void set_offset(const math::vec2& pOffset);
	math::vec2 get_offset() const;

	void set_size(const math::vec2& pSize);
	math::vec2 get_size() const;

	void set_rotation(math::radians pRads);
	math::radians get_rotation() const;

	void set_sensor(bool pIs_sensor);
	bool is_sensor() const;

	void set_anchor(math::vec2 pRatio);
	math::vec2 get_anchor() const;

	// Update the shape in the fixture (if it exists)
	void update_current_shape(const math::vec2& pScale);

protected:
	virtual json on_serialize(core::serialize_type) const override;
	virtual void on_deserialize(const core::game_object& pObject, const json&) override;

	// Update a shape to the current transform
	void update_shape(const math::vec2& pScale, b2PolygonShape* pShape);

private:
	b2Fixture* mFixture;
	math::vec2 mOffset;
	math::vec2 mSize;
	math::radians mRotation;
	bool mIs_sensor;
	math::vec2 mAnchor;

	friend class physics_world;
};

}