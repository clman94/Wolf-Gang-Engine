#version 330 core
layout(location = 0) in vec2 vertex_position;
layout(location = 1) in vec2 vertex_uv;

uniform mat4 projection;
uniform mat4 model;

out vec2 uv;

void main()
{
	uv = vertex_uv;
	gl_Position = projection * model * vec4(vertex_position, 0, 1.0);
}