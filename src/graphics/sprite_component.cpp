#include <wge/graphics/sprite_component.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/core/game_object.hpp>

namespace wge::graphics
{

void sprite_component::create_batch(math::transform& pTransform, renderer& pRenderer)
{
	// No sprite.
	if (!mSprite)
		return;

	const texture& sprite_texture = mSprite->get_texture();

	batch_builder batch;
	batch.set_texture(sprite_texture);

	const math::vec2 frame_size{ mSprite->get_frame_size() };

	math::aabb uv = mSprite->get_frame_uv(0);

	vertex_2d verts[4];
	verts[0].position = math::vec2(0, 0);
	verts[0].uv = uv.min;
	verts[1].position = frame_size.swizzle(math::_x, 0);
	verts[1].uv = math::vec2(uv.max.x, uv.min.y);
	verts[2].position = frame_size;
	verts[2].uv = uv.max;
	verts[3].position = frame_size.swizzle(0, math::_y);
	verts[3].uv = math::vec2(uv.min.x, uv.max.y);

	math::vec2 anchor = mSprite->get_frame_anchor(0);

	// Transform the vertices
	for (int i = 0; i < 4; i++)
	{
		verts[i].position += mOffset - anchor;
		verts[i].position *= pRenderer.get_pixel_size();
		
		// Calc aabb of sprite before transform
		if (i == 0)
			mLocal_aabb = math::aabb{ verts[i].position, verts[i].position };
		else
			mLocal_aabb.merge(verts[i].position);

		// Transform the points
		verts[i].position = pTransform * verts[i].position;

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

void sprite_component::set_sprite(const core::asset::ptr& pAsset) noexcept
{
	mSprite = pAsset;
}

core::asset::ptr sprite_component::get_sprite() const noexcept
{
	return mSprite.get_asset();
}

} // namespace wge::graphics
