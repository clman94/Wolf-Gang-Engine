#pragma once

#define STRINGIFY(A) #A

namespace wge::graphics::shaders
{

constexpr const char* fragment_color =  "#version 330 core\n" STRINGIFY(
	in vec4 color;

	out vec4 output_color;

	void main()
	{
		output_color = color;
	}
);

constexpr const char* fragment_texture = "#version 330 core\n" STRINGIFY(
	in vec2 uv;
	in vec4 color;

	out vec4 output_color;

	uniform sampler2D tex;

	void main()
	{
		output_color = texture(tex, uv)*color;
	}
);

constexpr const char* vertex_color = "#version 330 core\n" STRINGIFY(
	layout(location = 0) in vec2 vertex_position;
	layout(location = 2) in vec4 vertex_color;

	uniform mat4 projection;

	out vec4 color;

	void main()
	{
		color = vertex_color;
		gl_Position = projection * vec4(vertex_position, 0, 1.0);
	}
);

constexpr const char* vertex_texture = "#version 330 core\n" STRINGIFY(
	layout(location = 0) in vec2 vertex_position;
	layout(location = 1) in vec2 vertex_uv;
	layout(location = 2) in vec4 vertex_color;

	uniform mat4 projection;

	out vec2 uv;
	out vec4 color;

	void main()
	{
		uv = vertex_uv;
		color = vertex_color;
		gl_Position = projection * vec4(vertex_position, 0, 1.0);
	}
);

} // namespace wge::graphics::shaders

#undef STRINGIFY
