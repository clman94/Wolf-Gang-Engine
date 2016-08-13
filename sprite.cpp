#define ENGINE_INTERNAL

#include "renderer.hpp"

using namespace engine;

int sprite_node::draw(renderer &pR)
{
	sf::RenderStates rs;
	rs.transform.translate(get_exact_position() + mOffset);
	rs.texture = &mTexture->sfml_get_texture();
	rs.transform.scale(mScale);
	pR.mWindow.draw(&mVertices[0], 4, sf::Quads , rs);
	return 0;
}

void sprite_node::set_scale(fvector pScale)
{
	mScale = pScale;
}

int sprite_node::set_texture(texture& pTexture)
{
	mTexture = &pTexture;
	return 0;
}

int sprite_node::set_texture(texture& pTexture, std::string pAtlas)
{
	mTexture = &pTexture;
	auto crop = pTexture.get_entry(pAtlas);
	set_texture_rect(crop);
	return 0;
}

sprite_node::sprite_node()
{
	mScale = { 1, 1 };
	mTexture = nullptr;
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
sprite_node::set_texture_rect(const engine::frect& pRect)
{
	mVertices[0].texCoords = pRect.get_offset();
	mVertices[1].texCoords = pRect.get_offset() + fvector(pRect.w, 0);
	mVertices[2].texCoords = pRect.get_offset() + pRect.get_size();
	mVertices[3].texCoords = pRect.get_offset() + fvector(0, pRect.h);

	mVertices[0].position = { 0, 0 };
	mVertices[1].position = { pRect.w, 0 };
	mVertices[2].position = pRect.get_size();
	mVertices[3].position = { 0, pRect.h };
}