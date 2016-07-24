#define ENGINE_INTERNAL
#include "renderer.hpp"
#include <cassert>

using namespace engine;

static void vertex_set_position(sf::Vertex* v, fvector p, frect r)
{
	v[0].position = { p.x, p.y };
	v[1].position = { p.x + r.w, p.y };
	v[2].position = { p.x + r.w, p.y + r.h };
	v[3].position = { p.x, p.y + r.h };
}

static void vertex_set_texture(sf::Vertex* v, frect r)
{
	v[0].texCoords = { r.x, r.y };
	v[1].texCoords = { r.x + r.w, r.y };
	v[2].texCoords = { r.x + r.w, r.y + r.h };
	v[3].texCoords = { r.x, r.y + r.h };
}


void
sprite_batch::set_texture(texture &t)
{
	c_texture = &t;
}

size_t
sprite_batch::add_sprite(fvector pos, frect tex_rect)
{
	sf::Vertex v[4];

	vertex_set_position(v, pos, tex_rect);
	vertex_set_texture(v, tex_rect);

	vertices.push_back(v[0]);
	vertices.push_back(v[1]);
	vertices.push_back(v[2]);
	vertices.push_back(v[3]);
	return vertices.size() - 4;
}

void
sprite_batch::set_sprite_position(size_t index, fvector pos, frect tex_rect)
{
	vertex_set_position(&vertices[index], pos, tex_rect);
}

void
sprite_batch::set_sprite_texture(size_t index, frect tex_rect)
{
	assert(index < vertices.size());
	vertex_set_texture(&vertices[index], tex_rect);
}

fvector
sprite_batch::get_sprite_position(size_t index)
{
	assert(index < vertices.size());
	auto v = vertices[index].position;
	return{ v.x, v.y };
}

int
sprite_batch::draw(renderer &_r)
{
	if (!c_texture || !vertices.size())
		return 1;

	sf::RenderStates rs;
	rs.transform.translate({ get_position().x, get_position().y });
	rs.texture = &c_texture->sfml_get_texture();
	_r.get_sfml_window().draw(&vertices[0], vertices.size(), sf::Quads, rs);
	return 0;
}