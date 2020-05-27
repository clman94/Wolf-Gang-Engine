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
	const texture* rendertexture = nullptr;

	// This is the depth in which this batch will be drawn.
	// Batches with lower values are closer to the forground
	// and higher values are closer to the background.
	float depth{ 0 };

	// Primitive type to be drawn
	primitive_type type{ primitive_type::triangles };

	std::vector<unsigned int> indexes;
	std::vector<vertex_2d> vertices;

	bool use_indirect_source = false;
	util::span<const unsigned int> indexes_indirect;
	util::span<const vertex_2d> vertices_indirect;

	bool empty() const noexcept
	{
		return (!use_indirect_source && indexes.empty() && vertices.empty()) ||
			(use_indirect_source && indexes_indirect.empty() && vertices_indirect.empty());
	}
};

struct quad_vertices
{
	vertex_2d corners[4];

	void set_rect(const math::rect& pRect)
	{
		corners[0].position = pRect.get_corner(0);
		corners[1].position = pRect.get_corner(1);
		corners[2].position = pRect.get_corner(2);
		corners[3].position = pRect.get_corner(3);
	}

	void set_uv(const math::rect& pRect)
	{
		corners[0].uv = pRect.get_corner(0);
		corners[1].uv = pRect.get_corner(1);
		corners[2].uv = pRect.get_corner(2);
		corners[3].uv = pRect.get_corner(3);
	}
};

struct quad_indicies
{
	unsigned int corners[6];
	
	void set_start_index(std::size_t pIndex) noexcept
	{
		const unsigned int index = static_cast<unsigned int>(pIndex);
		corners[0] = index;
		corners[1] = index + 1;
		corners[2] = index + 2;
		corners[3] = index + 2;
		corners[4] = index + 3;
		corners[5] = index;
	}
};

} // namespace wge::graphics
