#version 330 core

in vec2 uv;

out vec4 output_color;

uniform sampler2D tex;

void main()
{
	output_color = texture(tex, uv);
}