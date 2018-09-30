#pragma once

#include <GL/glew.h>
#include <vector>

#include <wge/math/vector.hpp>
#include <wge/math/matrix.hpp>
#include <wge/graphics/color.hpp>
#include <wge/graphics/texture.hpp>

namespace wge::graphics
{

class framebuffer;

struct vertex_2d
{
	math::vec2 position;
	math::vec2 uv;
	color thiscolor{ 1, 1, 1, 1 };
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

class renderer
{
public:
	// Compile the default shaders and initialize opengl
	void initialize();

	// Add a batch to render
	void push_batch(const render_batch_2d& pBatch)
	{
		mBatches.push_back(pBatch);
	}

	// Renders all the batches.
	// All batches are cleared after this is called.
	void render();

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
	framebuffer* mFramebuffer{ nullptr };
	GLuint mShader_texture, mShader_color;
	GLuint mVertex_buffer, mElement_buffer, mVAO_id;
	math::mat44 mProjection_matrix;
	float mPixel_size{ 1 };

	std::vector<render_batch_2d> mBatches;
};

} // namespace wge::graphics