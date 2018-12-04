#pragma once

#include <wge/core/component.hpp>
#include <wge/math/vector.hpp>
#include <wge/math/matrix.hpp>

namespace wge::core
{

// 2D transform component
class transform_component :
	public component
{
	WGE_COMPONENT_SINGLE_INSTANCE("Transform 2D", 0);
public:
	transform_component(core::component_id pId);

	virtual json serialize() const override;
	virtual void deserialize(const json& pJson) override;

	// Set the position of this transform
	void set_position(const math::vec2& pVec);
	math::vec2 get_position() const;

	// Set the rotation in radians
	void set_rotaton(const math::radians& pRad);
	// Get the rotation in radians
	math::radians get_rotation() const;

	// Scale this transform
	void set_scale(const math::vec2& pVec);
	math::vec2 get_scale() const;

	// Get transform combining the position, rotation,
	// and scale properties.
	math::mat33 get_transform();

private:
	// Updates the transform
	void update_transform();

private:
	math::vec2 mPosition;
	math::radians mRotation;
	math::vec2 mScale;

	bool mTransform_needs_update;
	math::mat33 mTransform;
};

}
