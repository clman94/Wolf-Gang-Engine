#pragma once

#include <wge/math/vector.hpp>
#include <wge/graphics/color.hpp>
#include <wge/graphics/texture.hpp>

#include <vector>

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
	color color{ 1, 1, 1, 1 };
};

enum class primitive_type
{
	triangles,
	linestrip,
	triangle_fan
};

struct render_batch_2d
{
	// Texture associated with this batch.
	// If nullptr, the primitives will be drawn with
	// a flat color.
	texture::ptr rendertexture;

	// This is the depth in which this batch will be drawn.
	// Batches with lower values are closer to the forground
	// and higher values are closer to the background.
	float depth{ 0 };

	// Primitive type to be drawn
	primitive_type type{ primitive_type::triangles };

	std::vector<unsigned int> indexes;
	std::vector<vertex_2d> vertices;
};

} // namespace wge::graphics
