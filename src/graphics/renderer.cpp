#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <iostream>

#include <wge/graphics/renderer.hpp>
#include <wge/graphics/framebuffer.hpp>
#include <wge/math/transformations.hpp>

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

void renderer::set_render_view(const math::aabb& mAABB)
{
	mRender_view = mAABB;
}

void renderer::set_render_view_to_framebuffer(const math::vec2& pOffset, const math::vec2 & pScale)
{
	WGE_ASSERT(mFramebuffer);
	const math::vec2 framebuffer_size = {
		static_cast<float>(mFramebuffer->get_width()),
		static_cast<float>(mFramebuffer->get_height()) };
	set_render_view({ pOffset, pOffset + framebuffer_size * pScale });
}

math::aabb renderer::get_render_view() const
{
	return mRender_view;
}

math::vec2 renderer::get_render_view_scale() const
{
	return (mRender_view.max - mRender_view.min) / math::vec2((float)mFramebuffer->get_width(), (float)mFramebuffer->get_height());
}

math::vec2 renderer::world_to_screen(const math::vec2 & pVec) const
{
	return (pVec - mRender_view.min) * get_render_view_scale();
}

math::vec2 renderer::screen_to_world(const math::vec2 & pVec) const
{
	return (pVec / get_render_view_scale()) + mRender_view.max;
}

const std::vector<render_batch_2d>& renderer::collect_batches()
{
	get_layer().for_each(
		[&](sprite_component& pSprite, core::transform_component& pTransform)
	{
		pSprite.create_batch(pTransform, *this);
	});

	return mBatches;
}

void renderer::clear()
{
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
