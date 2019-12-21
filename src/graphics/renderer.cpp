#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <iostream>

#include <wge/graphics/renderer.hpp>
#include <wge/graphics/framebuffer.hpp>
#include <wge/math/transform.hpp>
#include <wge/math/transformations.hpp>
#include <wge/graphics/graphics.hpp>

namespace wge::graphics
{

std::size_t batch_builder::add_quad(const vertex_2d* pBuffer)
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
	for (auto [id, sprite, transform] :
		get_layer().each<sprite_component, math::transform>())
	{
		sprite.create_batch(transform, *this);
	}

	sort_batches();

	mProjection_matrix = math::ortho(mRender_view);
	for (const auto& i : mBatches)
		pGraphics.get_graphics_backend()->render_batch(mFramebuffer, mProjection_matrix, i);
	mBatches.clear();
}

void renderer::render_tilemap(graphics& pGraphics, core::resource_handle<texture> pTexture)
{
	std::size_t index = 0;
	for (auto [id, indexes] : get_layer().each<quad_indicies>())
	{
		indexes.set_start_index(index++ * 4);
	}
	
	render_batch_2d batch;
	batch.rendertexture = pTexture;
	for (auto& i : get_layer().get_storage<quad_vertices>().get_const_raw())
	{
		for (auto corner : i.corners)
			batch.vertices.push_back(corner);
	}
	auto thing = get_layer().get_storage<quad_indicies>().get_const_raw();
	for (auto& i : thing)
	{
		for (auto index : i.corners)
			batch.indexes.push_back(index);
	}
	pGraphics.get_graphics_backend()->render_batch(mFramebuffer, mProjection_matrix, batch);
}

json renderer::on_serialize(core::serialize_type pType)
{
	json result;
	if (pType & core::serialize_type::properties)
	{
		result["pixel-size"] = mPixel_size;
	}
	return result;
}

void renderer::on_deserialize(const json& pJson)
{
	mPixel_size = pJson["pixel-size"];
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
