#pragma once

#include <wge/graphics/color.hpp>

// Redefined here to move the gl header to a different file
typedef unsigned int GLuint;

namespace wge::graphics
{

// This class manages an opengl framebuffer object
// for easy render to texture functionality.
class framebuffer
{
public:
	framebuffer();
	~framebuffer();

	// Create the framebuffer and texture.
	// Use resize() to adjust the framebuffer size.
	void create(int pWidth, int pHeight);

	// Resize the framebuffer
	void resize(int pWidth, int pHeight);

	// This sets the frame buffer for opengl.
	// Call this first if you want to draw to this framebuffer.
	// Call end_framebuffer() when you are done.
	void begin_framebuffer() const;

	// Resets opengl back to the default framebuffer
	void end_framebuffer() const;

	void clear(const color& pColor);

	// Get the raw gl texture id
	GLuint get_gl_texture() const
	{
		return mTexture;
	}

	// Get width of framebuffer texture in pixels
	int get_width() const
	{
		return mWidth;
	}

	// Get height of framebuffer texture in pixels
	int get_height() const
	{
		return mHeight;
	}

private:
	GLuint mTexture, mFramebuffer;
	int mWidth, mHeight;
};

} // namespace wge::graphics
