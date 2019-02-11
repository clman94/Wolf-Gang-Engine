#include <wge/physics/box_collider_component.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/core/transform_component.hpp>
#include <wge/logging/log.hpp>

#include <Box2D/Box2D.h>

#include <iostream>

namespace wge::physics
{

box_collider_component::box_collider_component() :
	mFixture(nullptr),
	mSize(1, 1),
	mIs_sensor(false)
{
}

box_collider_component::~box_collider_component()
{
}

json box_collider_component::on_serialize(core::serialize_type pType) const
{
	json result;
	if (pType & core::serialize_type::properties)
	{
		result["offset"] = mOffset;
		result["size"] = mSize;
		result["rotation"] = mRotation;
		result["sensor"] = mIs_sensor;
	}
	return result;
}

void box_collider_component::on_deserialize(const core::game_object& pObject, const json & pJson)
{
	mOffset = pJson["offset"];
	mSize = pJson["size"];
	mRotation = pJson["rotation"];
	set_sensor(pJson["sensor"]);
}

void box_collider_component::set_offset(const math::vec2 & pOffset)
{
	mOffset = pOffset;
}

math::vec2 box_collider_component::get_offset() const
{
	return mOffset;
}

void box_collider_component::set_size(const math::vec2 & pSize)
{
	if (pSize.x > 0 && pSize.y > 0)
	{
		mSize = pSize;
	}
}

math::vec2 box_collider_component::get_size() const
{
	return mSize;
}

void box_collider_component::set_rotation(math::radians pRads)
{
	mRotation = pRads;
}

math::radians box_collider_component::get_rotation() const
{
	return mRotation;
}

void box_collider_component::set_sensor(bool pIs_sensor)
{
	mIs_sensor = pIs_sensor;
	if (mFixture)
		mFixture->SetSensor(pIs_sensor);
}

bool box_collider_component::is_sensor() const
{
	return mIs_sensor;
}

void box_collider_component::set_anchor(math::vec2 pRatio)
{
	mAnchor = pRatio;
}

math::vec2 box_collider_component::get_anchor() const
{
	return mAnchor;
}

void box_collider_component::update_shape(const math::vec2& pScale, b2PolygonShape* pShape)
{
	assert(pShape);

	math::vec2 skin_size(pShape->m_radius, pShape->m_radius);
	math::vec2 offset = mSize / 2 + mOffset + skin_size + mSize * mAnchor;
	math::vec2 h_size = (mSize - skin_size * 2) / 2;
	pShape->SetAsBox(h_size.x, h_size.y, b2Vec2(offset.x, offset.y), mRotation);
	for (int i = 0; i < 4; i++)
	{
		math::vec2 vec{ pShape->m_vertices[i].x, pShape->m_vertices[i].y };
		vec *= pScale;
		pShape->m_vertices[i] = b2Vec2{ vec.x, vec.y };
	}
}

void box_collider_component::update_current_shape(const math::vec2& pScale)
{
	if (mFixture)
	{
		update_shape(pScale, dynamic_cast<b2PolygonShape*>(mFixture->GetShape()));
		mFixture->GetBody()->ResetMassData();
	}
}

} // namespace wge::physics
