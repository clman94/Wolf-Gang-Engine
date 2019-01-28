#include <wge/core/transform_component.hpp>
#include <wge/core/game_object.hpp>

namespace wge::core
{

transform_component::transform_component(core::component_id pId) noexcept :
	core::component(pId)
{
}

json transform_component::on_serialize(serialize_type pType) const
{
	json result;
	if (pType & serialize_type::properties)
	{
		result["position"] = mTransform.position;
		result["rotation"] = mTransform.rotation;
		result["scale"] = mTransform.scale;
	}
	return result;
}

void transform_component::on_deserialize(const game_object& pObject, const json& pJson)
{
	mTransform.position = pJson["position"];
	mTransform.rotation = pJson["rotation"];
	mTransform.scale = pJson["scale"];
}

void transform_component::set_position(const math::vec2 & pVec) noexcept
{
	mTransform.position = pVec;
}

math::vec2 transform_component::get_position() const noexcept
{
	return mTransform.position;
}

void transform_component::move(const math::vec2& pDelta) noexcept
{
	mTransform.position += pDelta;
}

void transform_component::set_rotaton(const math::radians& pRad) noexcept
{
	mTransform.rotation = pRad;
}

math::radians transform_component::get_rotation() const noexcept
{
	return mTransform.rotation;
}

void transform_component::set_scale(const math::vec2 & pVec) noexcept
{
	mTransform.scale = pVec;
}

math::vec2 transform_component::get_scale() const noexcept
{
	return mTransform.scale;
}

void transform_component::set_transform(const math::transform& pTransform) noexcept
{
	mTransform = pTransform;
}

const math::transform& transform_component::get_transform() const noexcept
{
	return mTransform;
}

} // namespace wge::core
