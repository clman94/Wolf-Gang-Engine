#version 330 core

in vec2 uv;
in vec4 color;

out vec4 output_color;

void main()
{
	output_color = color;
}