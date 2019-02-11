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
		result["animation"] = mAnimation ? json(mAnimation->name) : json();
	}
	return result;
}

void sprite_component::on_deserialize(const core::game_object& pObj, const json& pJson)
{
	mOffset = pJson["offset"];
	if (!pJson["texture"].is_null())
	{
		auto asset_mgr = pObj.get_context().get_asset_manager();
		util::uuid id = pJson["texture"];
		mTexture = asset_mgr->get_asset<texture>(id);
	}
	if (!pJson["animation"].is_null())
	{
		std::string name = pJson["animation"];
		mAnimation = mTexture->get_animation(name);
	}
}

void sprite_component::create_batch(core::transform_component& pTransform, renderer& pRenderer)
{
	// No texture
	if (!mTexture)
		return;

	batch_builder batch;
	batch.set_texture(mTexture);

	const math::vec2 texture_size = mTexture->get_size();

	math::aabb uv {
		mAnimation->frame_rect.position / texture_size,
		(mAnimation->frame_rect.position + mAnimation->frame_rect.size) / texture_size
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

void sprite_component::set_animation(animation::ptr pAnimation) noexcept
{
	mAnimation = pAnimation;
}

void sprite_component::set_animation(const std::string& pName) noexcept
{
	WGE_ASSERT(mTexture);
	mAnimation = mTexture->get_animation(pName);
}

void sprite_component::set_texture(const texture::ptr& pAsset) noexcept
{
	mTexture = pAsset;
	mAnimation = mTexture->get_animation("Default");
}

texture::ptr sprite_component::get_texture() const noexcept
{
	return mTexture;
}

} // namespace wge::graphics
