#include <wge/graphics/sprite_component.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/graphics/graphics.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/core/object.hpp>

namespace wge::graphics
{

void sprite_component::create_batch(math::transform& pTransform, renderer& pRenderer)
{
	if (!mController.get_sprite())
		return;

	const std::size_t current_frame = get_controller().get_frame();

	const auto sprite = mController.get_sprite();

	const texture& sprite_texture = sprite->get_texture();

	batch_builder batch;
	batch.set_texture(sprite_texture);

	const math::vec2 frame_size{ sprite->get_frame_size() };

	math::aabb uv = sprite->get_frame_uv(current_frame);

	vertex_2d verts[4];
	verts[0].position = math::vec2(0, 0);
	verts[0].uv = uv.min;
	verts[1].position = { frame_size.x, 0 };
	verts[1].uv = math::vec2(uv.max.x, uv.min.y);
	verts[2].position = frame_size;
	verts[2].uv = uv.max;
	verts[3].position = { 0, frame_size.y};
	verts[3].uv = math::vec2(uv.min.x, uv.max.y);

	math::vec2 anchor = sprite->get_frame_anchor(current_frame);

	// Transform the vertices
	for (int i = 0; i < 4; i++)
	{
		verts[i].position += mOffset - anchor;
		verts[i].position *= pRenderer.get_pixel_scale();
		
		// Calc aabb of sprite before transform
		if (i == 0)
			mLocal_aabb = math::aabb{ verts[i].position, verts[i].position };
		else
			mLocal_aabb.merge(verts[i].position);

		// Transform the points
		verts[i].position = pTransform * verts[i].position;
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
	mController.set_sprite(pAsset);
}

core::asset::ptr sprite_component::get_sprite() const noexcept
{
	return mController.get_sprite().get_asset();
}

} // namespace wge::graphics
