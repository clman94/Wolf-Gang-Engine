#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <iostream>

#include <wge/graphics/renderer.hpp>
#include <wge/graphics/framebuffer.hpp>
#include <wge/math/transformations.hpp>

namespace wge::graphics
{

static std::string load_file_as_string(const std::string& pPath)
{
	std::ifstream stream(pPath.c_str());
	if (!stream.good())
	{
		std::cout << "Could not load file \"" << pPath << "\"\n";
		return{};
	}
	std::stringstream sstr;
	sstr << stream.rdbuf();
	return sstr.str();
}

static bool compile_shader(GLuint pGL_shader, const std::string & pSource)
{
	// Compile Shader
	const char * source_ptr = pSource.c_str();
	glShaderSource(pGL_shader, 1, &source_ptr, NULL);
	glCompileShader(pGL_shader);

	// Get log info
	GLint result = GL_FALSE;
	int log_length = 0;
	glGetShaderiv(pGL_shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(pGL_shader, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 0)
	{
		// A compilation error occured. Print the message.
		std::vector<char> message(log_length + 1);
		glGetShaderInfoLog(pGL_shader, log_length, NULL, &message[0]);
		printf("%s\n", &message[0]);
		return false;
	}

	return true;
}

static GLuint load_shaders(const std::string & pVertex_path, const std::string & pFragment_path)
{
	// Load sources
	std::string vertex_str = load_file_as_string(pVertex_path);
	std::string fragment_str = load_file_as_string(pFragment_path);

	if (vertex_str.empty() || fragment_str.empty())
		return 0;

	// Create the shader objects
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Compile the shaders
	compile_shader(vertex_shader, vertex_str);
	compile_shader(fragment_shader, fragment_str);

	// Link the shaders to a program
	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, vertex_shader);
	glAttachShader(program_id, fragment_shader);
	glLinkProgram(program_id);

	// Check for errors
	GLint result = GL_FALSE;
	int log_length = 0;
	glGetProgramiv(program_id, GL_LINK_STATUS, &result);
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 0)
	{
		// An error occured
		std::vector<char> message(log_length + 1);
		glGetProgramInfoLog(program_id, log_length, NULL, &message[0]);
		printf("%s\n", &message[0]);
		return 0;
	}

	// Cleanup

	glDetachShader(program_id, vertex_shader);
	glDetachShader(program_id, fragment_shader);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program_id;
}

std::size_t batch_builder::add_quad(const vertex_2d * pBuffer)
{
	std::size_t start_index = mBatch.vertices.size();

	// Make sure there is enough space
	mBatch.indexes.reserve(mBatch.indexes.size() + 6);

	// Triangle 1
	mBatch.indexes.push_back(start_index);
	mBatch.indexes.push_back(start_index + 1);
	mBatch.indexes.push_back(start_index + 2);

	// Triangle 2
	mBatch.indexes.push_back(start_index + 2);
	mBatch.indexes.push_back(start_index + 3);
	mBatch.indexes.push_back(start_index);

	// Add them
	mBatch.vertices.reserve(mBatch.vertices.size() + 4);
	for (std::size_t i = 0; i < 4; i++)
		mBatch.vertices.push_back(pBuffer[i]);

	return start_index;
}

void renderer::initialize()
{
	mShader_texture = load_shaders(
		"./editor/shaders/vert_texture.glsl",
		"./editor/shaders/frag_texture.glsl"
	);
	mShader_color = load_shaders(
		"./editor/shaders/vert_color.glsl",
		"./editor/shaders/frag_color.glsl"
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

void renderer::set_render_view(const math::aabb& mAABB)
{
	mRender_view = mAABB;
}

void renderer::set_render_view_to_framebuffer(const math::vec2& pOffset, const math::vec2 & pScale)
{
	WGE_ASSERT(mFramebuffer);
	const math::vec2 framebuffer_size = {
		static_cast<float>(mFramebuffer->get_width()),
		static_cast<float>(mFramebuffer->get_height()) };
	set_render_view({ pOffset, pOffset + framebuffer_size * pScale });
}

math::aabb renderer::get_render_view() const
{
	return mRender_view;
}

math::vec2 renderer::get_render_view_scale() const
{
	return (mRender_view.max - mRender_view.min) / math::vec2((float)mFramebuffer->get_width(), (float)mFramebuffer->get_height());
}

math::vec2 renderer::world_to_screen(const math::vec2 & pVec) const
{
	return (pVec - mRender_view.min) * get_render_view_scale();
}

math::vec2 renderer::screen_to_world(const math::vec2 & pVec) const
{
	return (pVec / get_render_view_scale()) + mRender_view.max;
}

void renderer::render()
{
	assert(mFramebuffer);

	for (std::size_t i = 0; i < get_layer().get_object_count(); i++)
	{
		core::transform_component* transform;
		sprite_component* sprite;
		if (get_layer().retrieve_components(get_layer().get_object(i), sprite, transform))
		{
			sprite->create_batch(*transform, *this);
		}
	}

	mFramebuffer->begin_framebuffer();

	glViewport(0, 0, mFramebuffer->get_width(), mFramebuffer->get_height());

	// Create the projection matrix
	mProjection_matrix = math::ortho(mRender_view);

	// Render the batches
	sort_batches();
	for (const render_batch_2d& i : mBatches)
		render_batch(i);

	// Cleanup
	mFramebuffer->end_framebuffer();
}

void renderer::clear()
{
	mBatches.clear();
}

void renderer::sort_batches()
{
	std::sort(mBatches.begin(), mBatches.end(),
		[](const render_batch_2d& l, const render_batch_2d& r)->bool
	{
		return l.depth > r.depth;
	});
}

void renderer::render_batch(const render_batch_2d & pBatch)
{
	glBindVertexArray(mVAO_id);

	// Populate the vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, mVertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, pBatch.vertices.size() * sizeof(vertex_2d), &pBatch.vertices[0], GL_STATIC_DRAW);

	// Populate the element buffer with index data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElement_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, pBatch.indexes.size() * sizeof(unsigned int), &pBatch.indexes[0], GL_STATIC_DRAW);

	GLuint current_shader = pBatch.rendertexture ? mShader_texture : mShader_color;
	glUseProgram(current_shader);

	// Set the matrix uniforms
	GLuint proj_id = glGetUniformLocation(current_shader, "projection");
	glUniformMatrix4fv(proj_id, 1, GL_FALSE, &mProjection_matrix.m[0][0]);

	if (pBatch.rendertexture)
	{
		// Setup the texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pBatch.rendertexture->get_gl_texture());
		GLuint tex_id = glGetUniformLocation(mShader_texture, "tex");
		glUniform1i(tex_id, 0);
	}

	// Setup the 2d position attribute
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mVertex_buffer);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)0);

	if (pBatch.rendertexture)
	{
		// Setup the UV attribute
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, mVertex_buffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)sizeof(math::vec2));
	}

	// Setup the color attribute
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, mVertex_buffer);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)(sizeof(math::vec2) * 2));

	// Bind the element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElement_buffer);

	glDrawElements(
		pBatch.type,
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
}

} // namespace wge::graphics
