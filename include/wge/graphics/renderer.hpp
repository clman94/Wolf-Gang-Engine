#pragma once

#include <GL/glew.h>
#include <vector>

#include <wge/math/aabb.hpp>
#include <wge/math/rect.hpp>
#include <wge/math/vector.hpp>
#include <wge/math/matrix.hpp>
#include <wge/graphics/color.hpp>
#include <wge/graphics/texture.hpp>
#include <wge/graphics/render_batch_2d.hpp>
#include <wge/core/system.hpp>
#include <wge/core/context.hpp>
#include <wge/graphics/sprite_component.hpp>
#include <wge/graphics/framebuffer.hpp>

namespace wge::graphics
{

class framebuffer;

class batch_builder
{
public:

	// TODO: Add ability to draw to several framebuffers for post-processing
	//void set_framebuffer(const std::string& pName);

	void set_texture(texture* pTexture)
	{
		mBatch.rendertexture = pTexture;
	}

	// Add 4 vertices as a quad. Returns the starting index in the vertices
	// member of the batch.
	std::size_t add_quad(const vertex_2d* pBuffer);

	render_batch_2d* get_batch()
	{
		return &mBatch;
	}

private:
	render_batch_2d mBatch;
};

class renderer :
	public core::system
{
	WGE_SYSTEM("Renderer", 43);
public:
	renderer(core::layer& pLayer) :
		core::system(pLayer)
	{
	}

	// Add a batch to be rendered
	void push_batch(const render_batch_2d& pBatch)
	{
		mBatches.push_back(pBatch);
	}

	void set_render_view(const math::aabb& mAABB);
	void set_render_view_to_framebuffer(const math::vec2& pOffset = { 0, 0 }, const math::vec2& pScale = { 1, 1 });
	math::aabb get_render_view() const;
	math::vec2 get_render_view_scale() const;

	// Convert world coordinates to screen coordinates
	math::vec2 world_to_screen(const math::vec2& pVec) const;
	// Convert screen coordinates to world coordinates
	math::vec2 screen_to_world(const math::vec2& pVec) const;

	const std::vector<render_batch_2d>& collect_batches();

	// Clear all batches for a new frame
	void clear();

	// Set the current frame buffer to render to
	void set_framebuffer(const framebuffer::ptr& pFramebuffer)
	{
		mFramebuffer = pFramebuffer;
	}
	framebuffer::ptr get_framebuffer() const
	{
		return mFramebuffer;
	}

	// Setting the pixel size allows you to adjust sprites
	// to any unit system you want. This is very helpful 
	// when you want to use physics which works with meters
	// per unit. Default: 1
	void set_pixel_size(float pSize)
	{
		mPixel_size = pSize;
	}
	float get_pixel_size() const
	{
		return mPixel_size;
	}

private:
	// Sort the batches so then the ones with greater depth are
	// farther in the background and less depth is closer to the
	// forground.
	void sort_batches();

private:
	math::aabb mRender_view;
	framebuffer::ptr mFramebuffer;
	math::mat44 mProjection_matrix;
	float mPixel_size{ 1 };

	std::vector<render_batch_2d> mBatches;
};

} // namespace wge::graphics
