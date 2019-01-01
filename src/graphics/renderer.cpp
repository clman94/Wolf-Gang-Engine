#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <iostream>

#include <wge/graphics/renderer.hpp>
#include <wge/graphics/framebuffer.hpp>
#include <wge/math/transformations.hpp>
#include <wge/graphics/graphics.hpp>

namespace wge::graphics
{

std::size_t batch_builder::add_quad(const vertex_2d * pBuffer)
{
	std::size_t start_index = mBatch.vertices.size();

	// Make sure there is enough space
	mBatch.indexes.reserve(mBatch.indexes.size() + 6);

	// Triangle 1
	mBatch.indexes.push_back(start_index);
	mBatch.indexes.push_back(start_index + 1);
	mBatch.indexes.push_back(start_index + 2);

	// Triangle 2
	mBatch.indexes.push_back(start_index + 2);
	mBatch.indexes.push_back(start_index + 3);
	mBatch.indexes.push_back(start_index);

	// Add them
	mBatch.vertices.reserve(mBatch.vertices.size() + 4);
	for (std::size_t i = 0; i < 4; i++)
		mBatch.vertices.push_back(pBuffer[i]);

	return start_index;
}

void renderer::set_render_view(const math::aabb& mAABB) noexcept
{
	mRender_view = mAABB;
}

void renderer::set_render_view_to_framebuffer(const math::vec2& pOffset, const math::vec2& pScale) noexcept
{
	WGE_ASSERT(mFramebuffer);
	set_render_view({ pOffset, pOffset + mFramebuffer->get_size() * pScale });
}

math::aabb renderer::get_render_view() const noexcept
{
	return mRender_view;
}

math::vec2 renderer::get_render_view_scale() const noexcept
{
	return (mRender_view.max - mRender_view.min) / mFramebuffer->get_size();
}

math::vec2 renderer::world_to_screen(const math::vec2& pVec) const noexcept
{
	return (pVec - mRender_view.min) * get_render_view_scale();
}

math::vec2 renderer::screen_to_world(const math::vec2& pVec) const noexcept
{
	return (pVec / get_render_view_scale()) + mRender_view.max;
}

void renderer::render(graphics& pGraphics)
{
	get_layer().for_each(
		[&](sprite_component& pSprite, core::transform_component& pTransform)
	{
		pSprite.create_batch(pTransform, *this);
	});

	sort_batches();

	mProjection_matrix = math::ortho(mRender_view);
	for (const auto& i : mBatches)
		pGraphics.get_graphics_backend()->render_batch(mFramebuffer, mProjection_matrix, i);
	mBatches.clear();
}

void renderer::sort_batches()
{
	std::sort(mBatches.begin(), mBatches.end(),
		[](const render_batch_2d& l, const render_batch_2d& r)->bool
	{
		return l.depth > r.depth;
	});
}

} // namespace wge::graphics
