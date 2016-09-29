#define ENGINE_INTERNAL

#include <engine/renderer.hpp>

#include <cassert>

using namespace engine;

void vertex_reference::set_position(fvector pPosition)
{
	auto ref = get_reference();
	sf::Vector2f offset = pPosition - fvector(ref[0].position);
	ref[0].position += offset;
	ref[1].position += offset;
	ref[2].position += offset;
	ref[3].position += offset;
}

fvector vertex_reference::get_position()
{
	auto ref = get_reference();
	return ref[0].position;
}

void vertex_reference::set_texture_rect(frect pRect, int pRotation)
{
	auto ref = get_reference();
	ref[(pRotation    ) % 4].texCoords = pRect.get_offset();
	ref[(pRotation + 1) % 4].texCoords = pRect.get_offset() + fvector(pRect.w, 0);
	ref[(pRotation + 2) % 4].texCoords = pRect.get_offset() + pRect.get_size();
	ref[(pRotation + 3) % 4].texCoords = pRect.get_offset() + fvector(0, pRect.h);
	reset_size(pRect.get_size());
}

void vertex_reference::reset_size(fvector pSize)
{
	auto ref = get_reference();
	fvector offset = ref[0].position;
	ref[1].position = offset + fvector(pSize.x, 0);
	ref[2].position = offset + pSize;
	ref[3].position = offset + fvector(0, pSize.y);
}

void vertex_reference::hide()
{
	auto ref = get_reference();
	ref[1] = ref[0];
	ref[2] = ref[0];
	ref[3] = ref[0];
}

sf::Vertex * vertex_reference::get_reference()
{
	assert(mBatch != nullptr);
	return &mBatch->mVertices[mIndex];
}

void
vertex_batch::set_texture(texture &t)
{
	mTexture = &t;
}

vertex_reference
vertex_batch::add_quad(fvector pPosition, frect pTexture_rect, int pRotation)
{
	mVertices.emplace_back();
	mVertices.emplace_back();
	mVertices.emplace_back();
	mVertices.emplace_back();

	vertex_reference ref;
	ref.mBatch = this;
	ref.mIndex = mVertices.size() - 4;
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
	_r.get_sfml_render().draw(&mVertices[0], mVertices.size(), sf::Quads, rs);
	return 0;
}

void vertex_batch::set_color(color pColor)
{
	for (auto& i : mVertices)
	{
		i.color = pColor;
	}
}
