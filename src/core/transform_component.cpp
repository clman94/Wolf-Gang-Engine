
#include <wge/core/transform_component.hpp>
#include <wge/core/object_node.hpp>

using namespace wge;
using namespace wge::core;

transform_component::transform_component(object_node * pObj) :
	component(pObj),
	mScale(1, 1),
	mRotation(0),
	mTransform_needs_update(true),
	mCache_needs_update(true)
{
	subscribe_to(pObj, "on_transform_changed", &transform_component::on_transform_changed, this);
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
	notify_transform_changed();
}

void transform_component::set_position(const math::vec2 & pVec)
{
	mPosition = pVec;
	mTransform_needs_update = true;
	notify_transform_changed();
}

math::vec2 transform_component::get_position() const
{
	return mPosition;
}

void transform_component::set_rotaton(const math::radians& pRad)
{
	mRotation = pRad;
	mTransform_needs_update = true;
	notify_transform_changed();
}

math::radians transform_component::get_rotation() const
{
	return mRotation;
}

void transform_component::set_scale(const math::vec2 & pVec)
{
	mScale = pVec;
	mTransform_needs_update = true;
	notify_transform_changed();
}

math::vec2 transform_component::get_scale() const
{
	return mScale;
}

math::vec2 transform_component::get_absolute_position()
{
	update_absolutes();
	return mCache_position;
}

math::radians transform_component::get_absolute_rotation()
{
	update_absolutes();
	return mCache_rotation;
}

math::vec2 transform_component::get_absolute_scale()
{
	update_absolutes();
	return mCache_scale;
}

math::mat33 transform_component::get_absolute_transform()
{
	update_absolutes();
	return mCache_transform;
}

math::mat33 transform_component::get_transform()
{
	update_transform();
	return mTransform;
}

void transform_component::update_absolutes()
{
	if (mCache_needs_update)
	{
		mCache_position = mPosition;
		mCache_rotation = mRotation;
		mCache_scale = mScale;

		update_transform();
		mCache_transform = mTransform;

		if (auto parent = get_object()->get_parent())
		{
			transform_component* transform = parent->get_component<transform_component>();
			if (transform)
			{
				mCache_scale *= transform->get_absolute_scale();
				mCache_rotation += transform->get_absolute_rotation();
				mCache_position = (mCache_position * transform->get_absolute_scale()).rotate(
					transform->get_absolute_rotation());
				mCache_transform = transform->get_absolute_transform() * mCache_transform;
			}
		}
		mCache_needs_update = false;
	}
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

void transform_component::on_transform_changed()
{
	mCache_needs_update = true;
}

void transform_component::notify_transform_changed()
{
	if (!mCache_needs_update)
		get_object()->send_down("on_transform_changed");
}
