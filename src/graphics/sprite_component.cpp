#include <wge/graphics/sprite_component.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/core/game_object.hpp>

namespace wge::graphics
{

json sprite_component::serialize() const
{
	json result;
	result["offset"] = { mOffset.x, mOffset.y };
	result["texture"] = mTexture ? json(mTexture->get_id()) : json();
	return result;
}

void sprite_component::deserialize(const json & pJson)
{
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

	// Get transform and scale it by the pixel size
	math::mat33 transform_mat = pTransform.get_transform();
	transform_mat.scale(math::vec2(pRenderer.get_pixel_size(), pRenderer.get_pixel_size()));

	// Transform the vertices
	for (int i = 0; i < 4; i++)
	{
		verts[i].position = transform_mat * (verts[i].position + (-mAnchor * texture_size) + mOffset);
		if (i == 0)
			mSceen_aabb = math::aabb{ verts[i].position, verts[i].position };
		else
			mSceen_aabb.merge(verts[i].position);
	}

	batch.add_quad(verts);

	pRenderer.push_batch(batch.finalize());
}

} // namespace wge::graphics
