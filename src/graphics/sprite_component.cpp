#include <wge/graphics/sprite_component.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/core/game_object.hpp>

namespace wge::graphics
{

json sprite_component::on_serialize(core::serialize_type pType) const
{
	json result;
	if (pType & core::serialize_type::properties)
	{
		result["offset"] = mOffset;
		result["texture"] = mTexture ? json(mTexture->get_id()) : json();

		texture::ptr res = mTexture->get_resource<texture>();
		const animation* anim = mTexture ? res->get_animation(mAnimation_id) : nullptr;
		result["animation-name"] = anim ? json(anim->name) : json();
		result["animation-id"] = anim ? json(anim->id) : json();
	}
	return result;
}

void sprite_component::on_deserialize(const core::game_object& pObj, const json& pJson)
{
	mOffset = pJson["offset"];
	if (!pJson["texture"].is_null())
	{
		auto& asset_mgr = pObj.get_context().get_asset_manager();
		util::uuid id = pJson["texture"];
		mTexture = asset_mgr.get_asset(id);
	}
	if (!pJson["animation-id"].is_null())
	{
		mAnimation_id = pJson["animation-id"];

		texture::ptr res = mTexture->get_resource<texture>();

		// Check if this id is correct
		if (!res->get_animation(mAnimation_id))
		{
			// Use the name of the animation as a backup
			std::string name = pJson["animation-name"];
			if (auto anim = res->get_animation(name))
				mAnimation_id = anim->id;
		}
	}
}

void sprite_component::create_batch(core::transform_component& pTransform, renderer& pRenderer)
{
	// No texture
	if (!mTexture)
		return;
	texture::ptr res = mTexture->get_resource<texture>();

	batch_builder batch;
	batch.set_texture(res);

	const math::vec2 texture_size = res->get_size();

	const animation* anim = res->get_animation(mAnimation_id);
	if (!anim)
		return;

	math::aabb uv {
		anim->frame_rect.position / texture_size,
		(anim->frame_rect.position + anim->frame_rect.size) / texture_size
	};

	vertex_2d verts[4];
	verts[0].position = math::vec2(0, 0);
	verts[0].uv = uv.min;
	verts[1].position = texture_size.swizzle(math::_x, 0);
	verts[1].uv = math::vec2(uv.max.x, uv.min.y);
	verts[2].position = texture_size;
	verts[2].uv = uv.max;
	verts[3].position = texture_size.swizzle(0, math::_y);
	verts[3].uv = math::vec2(uv.min.x, uv.max.y);

	// Transform the vertices
	for (int i = 0; i < 4; i++)
	{
		verts[i].position += -mAnchor * texture_size + mOffset;
		verts[i].position *= pRenderer.get_pixel_size();
		
		// Calc aabb of sprite before transform
		if (i == 0)
			mLocal_aabb = math::aabb{ verts[i].position, verts[i].position };
		else
			mLocal_aabb.merge(verts[i].position);

		// Transform the points
		verts[i].position = pTransform.get_transform() * verts[i].position;

		// Calc aabb after transform
		if (i == 0)
			mSceen_aabb = math::aabb{ verts[i].position, verts[i].position };
		else
			mSceen_aabb.merge(verts[i].position);
	}

	batch.add_quad(verts);

	pRenderer.push_batch(batch.finalize());
}

void sprite_component::set_offset(const math::vec2& pOffset) noexcept
{
	mOffset = pOffset;
}

math::vec2 sprite_component::get_offset() const noexcept
{
	return mOffset;
}

void sprite_component::set_anchor(const math::vec2& pRatio) noexcept
{
	mAnchor = pRatio;
}

math::vec2 sprite_component::get_anchor() const noexcept
{
	return mAnchor;
}

bool sprite_component::set_animation(const std::string& pName) noexcept
{
	WGE_ASSERT(mTexture);
	auto res = mTexture->get_resource<texture>();
	if (auto anim = res->get_animation(pName))
	{
		mAnimation_id = anim->id;
		return true;
	}
	return false;
}

bool sprite_component::set_animation(const util::uuid& pId) noexcept
{
	WGE_ASSERT(mTexture);
	auto res = mTexture->get_resource<texture>();
	if (res->get_animation(pId))
	{
		mAnimation_id = pId;
		return true;
	}
	return false;
}

void sprite_component::set_texture(const core::asset::ptr& pAsset) noexcept
{
	mTexture = pAsset;
	set_animation("Default");
}

core::asset::ptr sprite_component::get_texture() const noexcept
{
	return mTexture;
}

} // namespace wge::graphics
