#define ENGINE_INTERNAL
#include "renderer.hpp"
#include <cassert>

using namespace engine;

void vertex_reference::set_position(fvector pPosition)
{
	assert(mRef != nullptr);
	sf::Vector2f offset = pPosition - fvector(mRef[0].position);
	mRef[0].position += offset;
	mRef[1].position += offset;
	mRef[2].position += offset;
	mRef[3].position += offset;
}

fvector vertex_reference::get_position()
{
	assert(mRef != nullptr);
	return mRef[0].position;
}

void vertex_reference::set_texture_rect(frect pRect, int pRotation)
{
	assert(mRef != nullptr);
	mRef[(pRotation    ) % 4].texCoords = pRect.get_offset();
	mRef[(pRotation + 1) % 4].texCoords = pRect.get_offset() + fvector(pRect.w, 0);
	mRef[(pRotation + 2) % 4].texCoords = pRect.get_offset() + pRect.get_size();
	mRef[(pRotation + 3) % 4].texCoords = pRect.get_offset() + fvector(0, pRect.h);
	refresh_size();
}

void vertex_reference::refresh_size()
{
	assert(mRef != nullptr);
	sf::Vector2f offset = mRef[0].position;
	mRef[1].position = offset + (mRef[1].texCoords - mRef[0].texCoords);
	mRef[2].position = offset + (mRef[2].texCoords - mRef[0].texCoords);
	mRef[3].position = offset + (mRef[3].texCoords - mRef[0].texCoords);
}

void
vertex_batch::set_texture(texture &t)
{
	mTexture = &t;
}

vertex_reference
vertex_batch::add_sprite(fvector pPosition, frect pTexture_rect, int pRotation)
{
	mVertices.emplace_back();
	mVertices.emplace_back();
	mVertices.emplace_back();
	mVertices.emplace_back();
	vertex_reference ref = mVertices.at(mVertices.size() - 4);
	ref.set_texture_rect(pTexture_rect, pRotation);
	ref.set_position(pPosition);
	return ref;
}
int
vertex_batch::draw(renderer &_r)
{
	if (!mTexture || !mVertices.size())
		return 1;

	sf::RenderStates rs;
	rs.transform.translate(get_exact_position());
	rs.texture = &mTexture->sfml_get_texture();
	_r.get_sfml_window().draw(&mVertices[0], mVertices.size(), sf::Quads, rs);
	return 0;
}