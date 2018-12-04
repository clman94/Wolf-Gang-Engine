
#include <wge/core/transform_component.hpp>
#include <wge/core/game_object.hpp>

using namespace wge;
using namespace wge::core;

transform_component::transform_component(core::component_id pId) :
	core::component(pId),
	mScale(1, 1),
	mRotation(0),
	mTransform_needs_update(true)
{
}

json transform_component::serialize() const
{
	json result;
	result["position"] = { mPosition.x, mPosition.y };
	result["rotation"] = mRotation.value();
	result["scale"] = { mScale.x, mScale.y };
	return result;
}

void transform_component::deserialize(const json & pJson)
{
	mPosition = math::vec2(pJson["position"][0], pJson["position"][1]);
	mRotation = static_cast<float>(pJson["rotation"]);
	mScale = math::vec2(pJson["scale"][0], pJson["scale"][1]);
	mTransform_needs_update = true;
}

void transform_component::set_position(const math::vec2 & pVec)
{
	mPosition = pVec;
	mTransform_needs_update = true;
}

math::vec2 transform_component::get_position() const
{
	return mPosition;
}

void transform_component::set_rotaton(const math::radians& pRad)
{
	mRotation = pRad;
	mTransform_needs_update = true;
}

math::radians transform_component::get_rotation() const
{
	return mRotation;
}

void transform_component::set_scale(const math::vec2 & pVec)
{
	mScale = pVec;
	mTransform_needs_update = true;
}

math::vec2 transform_component::get_scale() const
{
	return mScale;
}

math::mat33 transform_component::get_transform()
{
	update_transform();
	return mTransform;
}

void transform_component::update_transform()
{
	if (mTransform_needs_update)
	{
		mTransform.identity();
		mTransform.translate(mPosition);
		mTransform.rotate(mRotation);
		mTransform.scale(mScale);
		mTransform_needs_update = false;
	}
}