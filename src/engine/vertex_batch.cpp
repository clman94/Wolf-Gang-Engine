#define ENGINE_INTERNAL

#include <engine/renderer.hpp>

#include <cassert>

using namespace engine;

vertex_reference::vertex_reference(const vertex_reference & A)
{
	mRotation = A.mRotation;
	mTexture_rect = A.mTexture_rect;
	mBatch = A.mBatch;
	mIndex = A.mIndex;
}

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

void vertex_reference::set_texture_rect(frect pRect)
{
	mTexture_rect = pRect;
	update_rect();
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

void vertex_reference::set_rotation(int pRotation)
{
	mRotation = std::abs(pRotation) % 4;
	update_rect();
}

int vertex_reference::get_rotation() const
{
	return mRotation;
}

void vertex_reference::update_rect()
{
	if (!mBatch)
		return;
	auto ref = get_reference();
	ref[(mRotation)     % 4].texCoords = mTexture_rect.get_offset();
	ref[(mRotation + 1) % 4].texCoords = mTexture_rect.get_offset() + fvector(mTexture_rect.w, 0);
	ref[(mRotation + 2) % 4].texCoords = mTexture_rect.get_offset() + mTexture_rect.get_size();
	ref[(mRotation + 3) % 4].texCoords = mTexture_rect.get_offset() + fvector(0, mTexture_rect.h);
}

sf::Vertex * vertex_reference::get_reference()
{
	assert(mBatch != nullptr);
	return &mBatch->mVertices[mIndex];
}

void
vertex_batch::set_texture(std::shared_ptr<texture> pTexture)
{
	mTexture = pTexture;
}

vertex_reference
vertex_batch::add_quad(fvector pPosition, frect pTexture_rect, int pRotation)
{
	for (size_t i = 0; i < 4; i++)
		mVertices.emplace_back();

	vertex_reference ref;
	ref.mBatch = this;
	ref.mIndex = mVertices.size() - 4;
	ref.set_rotation(pRotation);
	ref.set_texture_rect(pTexture_rect);
	ref.set_position(pPosition);
	return ref;
}

int
vertex_batch::draw(renderer &pR)
{
	if (!mTexture || !mVertices.size())
		return 1;

	sf::RenderStates rs;
	rs.transform.translate(get_exact_position());
	rs.texture = &mTexture->sfml_get_texture();
	if (mShader)
		rs.shader = mShader->get_sfml_shader();

	pR.get_sfml_render().draw(&mVertices[0], mVertices.size(), sf::Quads, rs);
	return 0;
}

void vertex_batch::set_color(color pColor)
{
	for (auto& i : mVertices)
	{
		i.color = pColor;
	}
}
