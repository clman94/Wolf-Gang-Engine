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

constexpr const char* fragment_texture = "#version 330 core\n" R"(
	in vec2 uv;
	in vec4 color;

	out vec4 output_color;

	uniform sampler2D tex;

	vec4 addition_sai(vec4 orig, vec4 d)
	{
		return orig * d + orig;
	}

	float lumi(vec3 color)
	{
		return 0.299 * color.x + 0.587 * color.y + 0.114 * color.z;
	}

	vec3 adjust_color_balance(vec3 value, vec3 shadows, vec3 midtones, vec3 highlights)
	{
		float lightness = lumi(value);

		const float a = 0.25, b = 0.333, scale = 0.7;

		shadows *= clamp((lightness - b) / -a + 0.5, 0.0, 1.0) * scale;
		midtones *= clamp((lightness - b) / a + 0.5, 0.0, 1.0) *
			clamp((lightness + b - 1.0) / -a + 0.5, 0.0, 1.0) * scale;
		highlights *= clamp((lightness + b - 1.0) / a + 0.5, 0.0, 1.0) * scale;

		value += shadows;
		value += midtones;
		value += highlights;
		//value = clamp(value, 0.0, 1.0);
		return value;
	}

	float adjust_value_balance(float value, float lightness, float shadows, float midtones, float highlights)
	{
		const float a = 0.25, b = 0.333, scale = 0.7;

		shadows *= clamp((lightness - b) / -a + 0.5, 0.0, 1.0) * scale;
		midtones *= clamp((lightness - b) / a + 0.5, 0.0, 1.0) *
			clamp((lightness + b - 1.0) / -a + 0.5, 0.0, 1.0) * scale;
		highlights *= clamp((lightness + b - 1.0) / a + 0.5, 0.0, 1.0) * scale;

		value += shadows;
		value += midtones;
		value += highlights;
		//value = clamp(value, 0.0, 1.0);
		return value;
	}

	void main()
	{
		vec4 full_color = texture(tex, uv) * color;
		output_color = full_color;
		//vec3(-0.3, -0.2, 0.05), vec3(-0.3, -0.2, 0.0), vec3(0.1, -0.2, 0)
		//output_color = vec4(adjust_color_balance(full_color.xyz, vec3(-0.3, -0.2, 0.1), vec3(0, 0, 0.0), vec3(0.1, 0.1, -0.1)), full_color.w);
		//output_color = addition_sai(texture(tex, uv)*color, vec4(1.2, 1.2, 0.5, 1));
	}
)";

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
