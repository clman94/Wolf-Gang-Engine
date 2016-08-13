#define ENGINE_INTERNAL

#include "renderer.hpp"

using namespace engine;

int sprite_node::draw(renderer &_r)
{
	sf::RenderStates rs;
	rs.transform.translate(get_exact_position() + mOffset);
	rs.texture = &c_texture->sfml_get_texture();
	rs.transform.scale(mScale);
	_r.window.draw(&mVertices[0], 4, sf::Quads , rs);
	return 0;
}

void sprite_node::set_scale(fvector pScale)
{
	mScale = pScale;
}

int sprite_node::set_texture(texture& pTexture)
{
	c_texture = &pTexture;
	return 0;
}

int sprite_node::set_texture(texture& pTexture, std::string pAtlas)
{
	c_texture = &pTexture;
	auto crop = pTexture.get_entry(pAtlas);
	set_texture_rect(crop);
	return 0;
}

void
sprite_node::set_anchor(anchor pType)
{
	mOffset = engine::anchor_offset(get_size(), pType);
}

fvector
sprite_node::get_size()
{
	return fvector(mVertices[2].texCoords - mVertices[0].texCoords)*mScale;
}

void
sprite_node::set_texture_rect(const engine::frect& rect)
{
	mVertices[0].texCoords = rect.get_offset();
	mVertices[1].texCoords = rect.get_offset() + fvector(rect.w, 0);
	mVertices[2].texCoords = rect.get_offset() + rect.get_size();
	mVertices[3].texCoords = rect.get_offset() + fvector(0, rect.h);
}