#pragma once

#include <GL/glew.h>
#include <vector>

#include <wge/math/aabb.hpp>
#include <wge/math/rect.hpp>
#include <wge/math/vector.hpp>
#include <wge/math/matrix.hpp>
#include <wge/graphics/color.hpp>
#include <wge/graphics/texture.hpp>
#include <wge/core/system.hpp>
#include <wge/core/context.hpp>
#include <wge/graphics/sprite_component.hpp>

namespace wge::graphics
{

class framebuffer;

struct vertex_2d
{
	// Represents the 2d coordinates of this vertex
	math::vec2 position;
	// UV position in the texture
	math::vec2 uv;
	// Color of vertex
	graphics::color color{ 1, 1, 1, 1 };
};

struct render_batch_2d
{
	// Texture associated with this batch.
	// If nullptr, the primitives will be drawn with
	// a flat color.
	texture* rendertexture{ nullptr };

	// This is the depth in which this batch will be drawn.
	// Batches with lower values are closer to the forground
	// and higher values are closer to the background.
	float depth{ 0 };

	enum primitive_type
	{
		type_triangles = GL_TRIANGLES,
		type_linestrip = GL_LINE_STRIP,
		type_triangle_fan = GL_TRIANGLE_FAN,
	};
	// Primitive type to be drawn
	primitive_type type{ type_triangles };

	std::vector<unsigned int> indexes;
	std::vector<vertex_2d> vertices;
};

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
		initialize();
	}

	// Compile the default shaders and initialize opengl
	void initialize();

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

	// Renders all the batches.
	void render();

	// Clear all batches for a new frame
	void clear();

	// Set the current frame buffer to render to
	void set_framebuffer(framebuffer* pFramebuffer)
	{
		mFramebuffer = pFramebuffer;
	}
	framebuffer* get_framebuffer() const
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

	// Render a single batch
	void render_batch(const render_batch_2d& pBatch);

private:
	math::aabb mRender_view;
	framebuffer* mFramebuffer{ nullptr };
	GLuint mShader_texture, mShader_color;
	GLuint mVertex_buffer, mElement_buffer, mVAO_id;
	math::mat44 mProjection_matrix;
	float mPixel_size{ 1 };

	std::vector<render_batch_2d> mBatches;
};

} // namespace wge::graphics
