#pragma once

#include <wge/graphics/color.hpp>
#include <wge/math/vector.hpp>
#include <memory>

// Redefined here to move the gl header to a different file
typedef unsigned int GLuint;

namespace wge::graphics
{

// This class manages an opengl framebuffer object
// for easy render to texture functionality.
class framebuffer
{
public:
	using ptr = std::shared_ptr<framebuffer>;

	virtual ~framebuffer() {}

	// Resize the framebuffer
	virtual void resize(int pWidth, int pHeight) = 0;

	// Clear the framebuffer vto a solid color
	virtual void clear(const color& pColor = { 0, 0, 0, 1 }) = 0;

	// Get width of framebuffer texture in pixels
	virtual int get_width() const = 0;
	// Get height of framebuffer texture in pixels
	virtual int get_height() const = 0;

	// Get a vec2 size of this framebuffer.
	math::vec2 get_size() const
	{
		return{ static_cast<float>(get_width()), static_cast<float>(get_height()) };
	}
};

} // namespace wge::graphics
