#define ENGINE_INTERNAL

#include <engine/renderer.hpp>

using namespace engine;

int sprite_node::draw(renderer &pR)
{
	if (!mTexture)
		return 1;

	const auto position = get_exact_position();

	sf::RenderStates rs;
	rs.transform.translate(position - mCenter);
	rs.transform.rotate(mRotation, mCenter);
	rs.texture = &mTexture->sfml_get_texture();
	rs.transform.scale(mScale);

	if (mShader)
		rs.shader = mShader->get_sfml_shader();

	pR.get_sfml_render().draw(&mVertices[0], 4, sf::Quads, rs);
	return 0;
}

void sprite_node::set_scale(fvector pScale)
{
	mScale = pScale;
	set_anchor(mAnchor);
}

fvector sprite_node::get_scale() const
{
	return mScale;
}

void sprite_node::set_texture(std::shared_ptr<texture> pTexture)
{
	mTexture = pTexture;
	set_anchor(mAnchor);
}

std::shared_ptr<texture> sprite_node::get_texture() const
{
	return mTexture;
}

sprite_node::sprite_node()
{
	mScale = { 1, 1 };
	mRotation = 0;
	mAnchor = anchor::topleft;
}

void
sprite_node::set_anchor(anchor pType)
{
	mCenter = engine::center_offset(get_size(), pType);
	mAnchor = pType;
}

fvector
sprite_node::get_size() const
{
	return fvector(mVertices[2].texCoords - mVertices[0].texCoords)*mScale;
}

float sprite_node::get_rotation() const
{
	return mRotation;
}

void sprite_node::set_shader(std::shared_ptr<shader> pShader)
{
	mShader = pShader;
}

frect sprite_node::get_render_rect() const
{
	return{ get_exact_position() + engine::anchor_offset(get_size(), mAnchor), get_size() };
}

void
sprite_node::set_texture_rect(const engine::frect& pRect)
{
	const engine::fvector offset(0.00005f, 0.00005f);

	mVertices[0].texCoords = pRect.get_offset() + offset;
	mVertices[1].texCoords = pRect.get_offset() + fvector(pRect.w, 0) + offset*engine::fvector(-1, 1);
	mVertices[2].texCoords = pRect.get_offset() + pRect.get_size() - offset;
	mVertices[3].texCoords = pRect.get_offset() + fvector(0, pRect.h) + offset*engine::fvector(1, -1);

	mVertices[0].position = { 0, 0 };
	mVertices[1].position = { pRect.w, 0 };
	mVertices[2].position = pRect.get_size();
	mVertices[3].position = { 0, pRect.h };

	set_anchor(mAnchor);
}

void sprite_node::set_color(color pColor)
{
	mVertices[0].color = pColor;
	mVertices[1].color = pColor;
	mVertices[2].color = pColor;
	mVertices[3].color = pColor;
}

void sprite_node::set_rotation(float pRotation)
{
	mRotation = pRotation;
}
