#pragma once

#include <wge/graphics/texture.hpp>
#include <GL/glew.h>

namespace wge::graphics
{

class opengl_texture_impl :
	public texture_impl
{
public:
	virtual ~opengl_texture_impl()
	{
		glDeleteTextures(1, &mGL_texture);
	}

	virtual void create_from_pixels(unsigned char* pBuffer, int pWidth, int pHeight, int mChannels) override
	{
		if (!mGL_texture)
		{
			// Create the texture object
			glGenTextures(1, &mGL_texture);
		}

		// Give the image to OpenGL
		glBindTexture(GL_TEXTURE_2D, mGL_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pWidth, pHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pBuffer);
		glBindTexture(GL_TEXTURE_2D, 0);

		update_filtering();
	}

	virtual void set_smooth(bool pSmooth) override
	{
		mSmooth = pSmooth;
		update_filtering();
	}

	virtual bool is_smooth() const override
	{
		return mSmooth;
	}

	GLuint get_gl_texture() const
	{
		return mGL_texture;
	}

private:
	void update_filtering()
	{
		glBindTexture(GL_TEXTURE_2D, mGL_texture);

		GLint filtering = mSmooth ? GL_LINEAR : GL_NEAREST;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

private:
	bool mSmooth{ false };
	GLuint mGL_texture;
};

} // namespace wge::graphics
