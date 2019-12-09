#pragma once

#include <iostream>
#include <string>
#include <cassert>
#include <fstream>
#include <wge/logging/log.hpp>

namespace wge::graphics
{

inline bool compile_shader(GLuint pGL_shader, const std::string& pSource)
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
		log::error("Error compiling shader: {}", &message[0]);
		return false;
	}

	return true;
}

inline GLuint load_shaders(const std::string& pVertex_source, const std::string& pFragment_source)
{
	if (pVertex_source.empty() || pFragment_source.empty())
		return 0;

	// Create the shader objects
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Compile the shaders
	compile_shader(vertex_shader, pVertex_source);
	compile_shader(fragment_shader, pFragment_source);

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
		log::error("Error linking shaders: {}", &message[0]);
		return 0;
	}

	// Cleanup

	glDetachShader(program_id, vertex_shader);
	glDetachShader(program_id, fragment_shader);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program_id;
}

} // namespace wge::graphics
