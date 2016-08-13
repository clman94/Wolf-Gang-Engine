#define ENGINE_INTERNAL
#include "renderer.hpp"
#include <cassert>

using namespace engine;

void vertex_reference::set_position(fvector position)
{
	assert(ref != nullptr);
	sf::Vector2f offset = position - fvector(ref[0].position);
	ref[0].position += offset;
	ref[1].position += offset;
	ref[2].position += offset;
	ref[3].position += offset;
}

fvector vertex_reference::get_position()
{
	assert(ref != nullptr);
	return ref[0].position;
}

void vertex_reference::set_texture_rect(frect rect, int rotation)
{
	assert(ref != nullptr);
	ref[(rotation) % 4].texCoords = rect.get_offset();
	ref[(rotation + 1) % 4].texCoords = rect.get_offset() + fvector(rect.w, 0);
	ref[(rotation + 2) % 4].texCoords = rect.get_offset() + rect.get_size();
	ref[(rotation + 3) % 4].texCoords = rect.get_offset() + fvector(0, rect.h);
	refresh_size();
}

void vertex_reference::refresh_size()
{
	assert(ref != nullptr);
	sf::Vector2f offset = ref[0].position;
	ref[1].position = offset + (ref[1].texCoords - ref[0].texCoords);
	ref[2].position = offset + (ref[2].texCoords - ref[0].texCoords);
	ref[3].position = offset + (ref[3].texCoords - ref[0].texCoords);
}

void
vertex_batch::set_texture(texture &t)
{
	c_texture = &t;
}

vertex_reference
vertex_batch::add_sprite(fvector pos, frect tex_rect, int rotation)
{
	vertices.emplace_back();
	vertices.emplace_back();
	vertices.emplace_back();
	vertices.emplace_back();
	vertex_reference ref = vertices.at(vertices.size() - 4);
	ref.set_texture_rect(tex_rect, rotation);
	ref.set_position(pos);
	return ref;
}
int
vertex_batch::draw(renderer &_r)
{
	if (!c_texture || !vertices.size())
		return 1;

	sf::RenderStates rs;
	rs.transform.translate(get_exact_position());
	rs.texture = &c_texture->sfml_get_texture();
	_r.get_sfml_window().draw(&vertices[0], vertices.size(), sf::Quads, rs);
	return 0;
}