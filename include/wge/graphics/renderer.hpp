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
#include <wge/core/scene.hpp>
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

	void set_texture(const texture::handle& pTexture) noexcept
	{
		mBatch.rendertexture = pTexture;
	}

	// Add 4 vertices as a quad. Returns the starting index in the vertices
	// member of the batch.
	std::size_t add_quad(const vertex_2d* pBuffer);

	render_batch_2d finalize() noexcept
	{
		return std::move(mBatch);
	}

private:
	render_batch_2d mBatch;
};

class graphics;

class renderer
{
public:

	// Add a batch to be rendered
	void push_batch(const render_batch_2d& pBatch)
	{
		mBatches.push_back(pBatch);
	}

	void set_render_view(const math::aabb& mAABB) noexcept;
	void set_render_view_to_framebuffer(const math::vec2& pOffset = { 0, 0 }, const math::vec2& pScale = { 1, 1 }) noexcept;
	math::aabb get_render_view() const noexcept;
	math::vec2 get_render_view_scale() const noexcept;

	// Convert world coordinates to screen coordinates
	[[nodiscard]] math::vec2 world_to_screen(const math::vec2& pVec) const noexcept;
	// Convert screen coordinates to world coordinates
	[[nodiscard]] math::vec2 screen_to_world(const math::vec2& pVec) const noexcept;

	void render(core::layer& pLayer, graphics& pGraphics);

	void render_tilemap(core::layer& pLayer, graphics& pGraphics, core::resource_handle<texture> pTexture);

	// Set the current frame buffer to render to
	void set_framebuffer(const framebuffer::ptr& pFramebuffer) noexcept
	{
		mFramebuffer = pFramebuffer;
	}
	framebuffer::ptr get_framebuffer() const noexcept
	{
		return mFramebuffer;
	}

	// Setting the pixel size allows you to adjust sprites
	// to any unit system you want. This is very helpful 
	// when you want to use physics which works with meters
	// per unit. Default: 1
	void set_pixel_size(float pSize) noexcept
	{
		mPixel_size = pSize;
	}
	float get_pixel_size() const noexcept
	{
		return mPixel_size;
	}

protected:
	virtual json on_serialize(core::serialize_type pType);
	virtual void on_deserialize(const json&);

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
