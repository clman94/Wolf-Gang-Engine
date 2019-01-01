#include <wge/core/transform_component.hpp>
#include <wge/core/game_object.hpp>

namespace wge::core
{

transform_component::transform_component(core::component_id pId) noexcept :
	core::component(pId),
	mScale(1, 1),
	mRotation(0),
	mTransform_needs_update(true)
{
}

json transform_component::on_serialize(serialize_type pType) const
{
	json result;
	if (pType & serialize_type::properties)
	{
		result["position"] = mPosition;
		result["rotation"] = mRotation;
		result["scale"] = mScale;
	}
	return result;
}

void transform_component::on_deserialize(const json& pJson)
{
	mPosition = pJson["position"];
	mRotation = pJson["rotation"];
	mScale = pJson["scale"];
	mTransform_needs_update = true;
}

void transform_component::set_position(const math::vec2 & pVec) noexcept
{
	mPosition = pVec;
	mTransform_needs_update = true;
}

math::vec2 transform_component::get_position() const noexcept
{
	return mPosition;
}

void transform_component::move(const math::vec2& pDelta) noexcept
{
	mPosition += pDelta;
	mTransform_needs_update = true;
}

void transform_component::set_rotaton(const math::radians& pRad) noexcept
{
	mRotation = pRad;
	mTransform_needs_update = true;
}

math::radians transform_component::get_rotation() const noexcept
{
	return mRotation;
}

void transform_component::set_scale(const math::vec2 & pVec) noexcept
{
	mScale = pVec;
	mTransform_needs_update = true;
}

math::vec2 transform_component::get_scale() const noexcept
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

} // namespace wge::core
