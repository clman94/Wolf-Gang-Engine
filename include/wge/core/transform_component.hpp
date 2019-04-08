#pragma once

#include <wge/core/component.hpp>
#include <wge/math/vector.hpp>
#include <wge/math/transform.hpp>

namespace wge::core
{

// 2D transform component
class transform_component :
	public component
{
	WGE_COMPONENT_SINGLE_INSTANCE("Transform 2d", 1);
public:
	// Set the position of this transform
	void set_position(const math::vec2& pVec) noexcept;
	math::vec2 get_position() const noexcept;

	void move(const math::vec2& pDelta) noexcept;

	// Set the rotation in radians
	void set_rotaton(const math::radians& pRad) noexcept;
	// Get the rotation in radians
	math::radians get_rotation() const noexcept;

	// Scale this transform
	void set_scale(const math::vec2& pVec) noexcept;
	math::vec2 get_scale() const noexcept;

	void set_transform(const math::transform& pTransform) noexcept;
	// Get transform combining the position, rotation,
	// and scale properties.
	const math::transform& get_transform() const noexcept;

protected:
	virtual json on_serialize(serialize_type) const override;
	virtual void on_deserialize(const game_object& pObject, const json&) override;

private:
	math::transform mTransform;
};

}
