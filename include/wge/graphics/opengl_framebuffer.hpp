#pragma once

#include <wge/graphics/framebuffer.hpp>

using GLuint = unsigned int;

namespace wge::graphics
{

class opengl_framebuffer :
	public framebuffer
{
public:
	virtual ~opengl_framebuffer();

	void create(int pWidth, int pHeight);

	virtual void resize(int pWidth, int pHeight) override;

	// Get width of framebuffer texture in pixels
	virtual int get_width() const override;

	// Get height of framebuffer texture in pixels
	virtual int get_height() const override;

	virtual void clear(const color& pColor) override;

	GLuint get_gl_texture() const;

	// This sets the frame buffer for opengl.
	// Call this first if you want to draw to this framebuffer.
	// Call end_framebuffer() when you are done.
	void begin_framebuffer() const;

	// Resets opengl back to the default framebuffer
	void end_framebuffer() const;

private:
	GLuint mTexture{ 0 }, mFramebuffer{ 0 };
	int mWidth{ 0 }, mHeight{ 0 };
};

} // namespace wge::graphics
