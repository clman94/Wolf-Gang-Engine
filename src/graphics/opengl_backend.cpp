#include <wge/graphics/graphics_backend.hpp>
#include <wge/graphics/opengl_framebuffer.hpp>
#include <wge/graphics/opengl_texture.hpp>
#include <wge/logging/log.hpp>
#include "shader_util.hpp"
#include "opengl_shaders.hpp"

#include <GL/glew.h>

namespace wge::graphics
{

inline GLenum primitive_type_to_opengl(primitive_type pType)
{
	switch (pType)
	{
	case primitive_type::triangles: return GL_TRIANGLES;
	case primitive_type::linestrip: return GL_LINE_STRIP;
	case primitive_type::triangle_fan: return GL_TRIANGLE_FAN;
	default: return GL_TRIANGLES;
	}
}

inline void GLAPIENTRY opengl_message_callback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH: log::out << log::level::error; break;
	case GL_DEBUG_SEVERITY_MEDIUM: log::out << log::level::warning; break;
	case GL_DEBUG_SEVERITY_LOW: log::out << log::level::warning; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: log::out << log::level::info; break;
	default: log::out << log::level::unknown;
	}
	log::out << "OpenGL: " << message << log::endm;
}

class opengl_backend_impl :
	public graphics_backend
{
public:
	virtual ~opengl_backend_impl()
	{
		glDeleteBuffers(1, &mVertex_buffer);
		glDeleteBuffers(1, &mElement_buffer);
		glDeleteVertexArrays(1, &mVAO_id);
		glDeleteProgram(mShader_texture);
		glDeleteProgram(mShader_color);
	}

	virtual void initialize() override
	{
		// Load extensions
		glewInit();

		// Enable blending (for transparancy)
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDebugMessageCallback(opengl_message_callback, 0);

		mShader_texture = load_shaders(
			shaders::vertex_texture,
			shaders::fragment_texture
		);
		mShader_color = load_shaders(
			shaders::vertex_color,
			shaders::fragment_color
		);

		glGenVertexArrays(1, &mVAO_id);
		glBindVertexArray(mVAO_id);

		// Create the needed vertex buffer
		glGenBuffers(1, &mVertex_buffer);

		// Allocate ahead of time
		glBindBuffer(GL_ARRAY_BUFFER, mVertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_2d) * 4, NULL, GL_STATIC_DRAW);

		// Create the element buffer to hold our indexes
		glGenBuffers(1, &mElement_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElement_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6, NULL, GL_STATIC_DRAW);
	}

	virtual void render_batch(const framebuffer::ptr& mFramebuffer, const math::mat44& pProjection, const render_batch_2d& pBatch) override
	{
		auto ogl_framebuffer = std::dynamic_pointer_cast<opengl_framebuffer>(mFramebuffer);
		if (!ogl_framebuffer)
			return;

		ogl_framebuffer->begin_framebuffer();

		glViewport(0, 0, ogl_framebuffer->get_width(), ogl_framebuffer->get_height());

		glBindVertexArray(mVAO_id);

		// Populate the vertex buffer.
		glBindBuffer(GL_ARRAY_BUFFER, mVertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, pBatch.vertices.size() * sizeof(vertex_2d), &pBatch.vertices[0], GL_STATIC_DRAW);

		// Populate the element buffer with index data.
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElement_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, pBatch.indexes.size() * sizeof(unsigned int), &pBatch.indexes[0], GL_STATIC_DRAW);

		GLuint current_shader = pBatch.rendertexture ? mShader_texture : mShader_color;
		glUseProgram(current_shader);

		// Set the matrix uniforms.
		GLuint proj_id = glGetUniformLocation(current_shader, "projection");
		glUniformMatrix4fv(proj_id, 1, GL_FALSE, &pProjection.m[0][0]);

		if (pBatch.rendertexture)
		{
			// Setup the texture.
			glActiveTexture(GL_TEXTURE0);
			auto impl = std::dynamic_pointer_cast<opengl_texture_impl>(pBatch.rendertexture->get_implementation());
			glBindTexture(GL_TEXTURE_2D, impl->get_gl_texture());
			GLuint tex_id = glGetUniformLocation(mShader_texture, "tex");
			glUniform1i(tex_id, 0);
		}

		// Setup the 2d position attribute.
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, mVertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)0);

		if (pBatch.rendertexture)
		{
			// Setup the UV attribute.
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, mVertex_buffer);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)sizeof(math::vec2));
		}

		// Setup the color attribute.
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, mVertex_buffer);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)(sizeof(math::vec2) * 2));

		// Bind the element buffer.
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElement_buffer);

		glDrawElements(
			primitive_type_to_opengl(pBatch.type),
			pBatch.indexes.size(),
			GL_UNSIGNED_INT,
			(void*)0
		);

		// Disable all the attributes
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);

		glUseProgram(0);

		glBindVertexArray(0);

		ogl_framebuffer->end_framebuffer();
	}

	virtual framebuffer::ptr create_framebuffer() override
	{
		auto ogl_framebuffer = std::make_shared<opengl_framebuffer>();
		ogl_framebuffer->create(200, 200); // Some arbitrary default
		return ogl_framebuffer;
	}

	virtual texture_impl::ptr create_texture_impl() override
	{
		return std::make_shared<opengl_texture_impl>();
	}

private:
	GLuint mVertex_buffer{ 0 }, mElement_buffer{ 0 }, mVAO_id{ 0 };
	GLuint mShader_texture{ 0 }, mShader_color{ 0 };
};

graphics_backend::ptr create_opengl_backend()
{

	log::info() << "Using OpenGL backend" << log::endm;
	return std::make_shared<opengl_backend_impl>();
}

} // namespace wge::graphics
