#version 330 core

layout(location = 0) in vec2 vertex_position;
layout(location = 2) in vec4 vertex_color;

uniform mat4 projection;

out vec4 color;

void main()
{
	color = vertex_color;
	gl_Position = projection * vec4(vertex_position, 0, 1.0);
}