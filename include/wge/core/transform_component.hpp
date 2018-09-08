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
	WGE_COMPONENT("Transform 2D", 0);
public:
	transform_component(object_node* pObj);

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

	// Combines all of the transforms of this objects
	// parents. Use these for rendering.
	math::vec2 get_absolute_position();
	math::radians get_absolute_rotation();
	math::vec2 get_absolute_scale();
	math::mat33 get_absolute_transform();

private:
	// Updates the the cached absolute values
	void update_absolutes();
	// Updates the transform
	void update_transform();
	
	// This event is recieved when the transform of this component
	// or any of its parents change. When any of the get_absolute_*() methods
	// are called, the cache will be updated.
	void on_transform_changed();

	// This will notify all components and the children nodes
	// that this node has changed position and they need to 
	// update their cache.
	void notify_transform_changed();

private:
	math::vec2 mPosition;
	math::radians mRotation;
	math::vec2 mScale;

	bool mTransform_needs_update;
	math::mat33 mTransform;

	// These are cached values for the absolute transform values
	bool mCache_needs_update;
	math::vec2 mCache_position;
	math::radians mCache_rotation;
	math::vec2 mCache_scale;
	math::mat33 mCache_transform;
};

}
