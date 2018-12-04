#include <wge/graphics/framebuffer.hpp>

#include <GL/glew.h>
#include <iostream>

namespace wge::graphics
{

framebuffer::framebuffer()
{
	mWidth = 0;
	mHeight = 0;
	mTexture = 0;
}

framebuffer::~framebuffer()
{
	glDeleteTextures(1, &mTexture);
	glDeleteFramebuffers(1, &mFramebuffer);
}

void framebuffer::create(int pWidth, int pHeight)
{
	if (pWidth <= 0 || pHeight <= 0 || mTexture != 0)
		return;

	// Create the framebuffer object
	glGenFramebuffers(1, &mFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);

	// Create the texture
	glGenTextures(1, &mTexture);
	glBindTexture(GL_TEXTURE_2D, mTexture);

	// Create the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pWidth, pHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	// GL_NEAREST will tell the sampler to get the nearest pixel when
	// rendering rather than lerping the pixels together.
	// It may be better to use a GL_LINEAR filter so then upscaling and downscaling
	// arn't so jaring.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// Attach the texture to #0
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTexture, 0);

	// Set the draw buffer
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Incomplete framebuffer\n";
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	mWidth = pWidth;
	mHeight = pHeight;
}

void framebuffer::resize(int pWidth, int pHeight)
{
	if (pWidth > 0 && pHeight > 0)
	{
		// Re-allocate the texture but with the new size
		glBindTexture(GL_TEXTURE_2D, mTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pWidth, pHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		mWidth = pWidth;
		mHeight = pHeight;
	}
}

void framebuffer::begin_framebuffer() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
}

void framebuffer::end_framebuffer() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void framebuffer::clear(const color& pColor)
{
	glClearColor(pColor.r, pColor.g, pColor.b, pColor.a);
	glClear(GL_COLOR_BUFFER_BIT);
}

} // namespace wge::graphics
