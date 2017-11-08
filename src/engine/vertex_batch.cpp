#define ENGINE_INTERNAL

#include <engine/renderer.hpp>

#include <cassert>

using namespace engine;


void vertex_reference::set_position(fvector pPosition)
{
	mRect.set_offset(pPosition);
	update_position();
}

fvector vertex_reference::get_position()
{
	return mRect.get_offset();
}

void vertex_reference::set_texture_rect(frect pRect)
{
	mTexture_rect = pRect;
	update_texture();
	set_size(pRect.get_size());
}

void vertex_reference::set_size(fvector pSize)
{
	mRect.set_size(pSize);
	update_position();
}

fvector vertex_reference::get_size() const
{
	return mRect.get_size();
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
	update_position();
	update_texture();
}

int vertex_reference::get_rotation() const
{
	return mRotation;
}

void vertex_reference::set_hskew(float pPercent)
{
	mHskew = pPercent;
}

void vertex_reference::set_color(const color & pColor)
{
	mColor = pColor;
	update_color();
}

void vertex_reference::update_position()
{
	if (!mBatch)
		return;

	fvector positions[4];

	// Offset
	for (size_t i = 0; i < 4; i++)
		positions[i] = mRect.get_offset();

	// Size and rotation
	if (mRotation % 2 == 0) // 0 or 180 degrees
	{
		positions[1] += fvector::x_only(mRect.get_size());
		positions[2] += mRect.get_size();
		positions[3] += fvector::y_only(mRect.get_size());
	}
	else // 90 or -90 degrees
	{
		positions[1] += fvector(mRect.get_size().y, 0);
		positions[2] += fvector(mRect.get_size().y, mRect.get_size().x);
		positions[3] += fvector(0, mRect.get_size().x);
	}
	
	// Skew
	const fvector hskew_offset(mRect.get_size().x*mHskew*0.5f, 0);
	positions[0] += hskew_offset;
	positions[1] += hskew_offset;
	positions[2] -= hskew_offset;
	positions[3] -= hskew_offset;

	auto ref = get_reference();
	for (size_t i = 0; i < 4; i++)
		ref[i].position = sf::Vector2f(positions[i]);
}

void vertex_reference::update_texture()
{
	if (!mBatch)
		return;
	auto ref = get_reference();
	ref[(mRotation    ) % 4].texCoords = mTexture_rect.get_offset();
	ref[(mRotation + 1) % 4].texCoords = mTexture_rect.get_offset() + fvector(mTexture_rect.w, 0);
	ref[(mRotation + 2) % 4].texCoords = mTexture_rect.get_offset() + mTexture_rect.get_size();
	ref[(mRotation + 3) % 4].texCoords = mTexture_rect.get_offset() + fvector(0, mTexture_rect.h);
}

void vertex_reference::update_color()
{
	if (!mBatch)
		return;
	auto ref = get_reference();
	for (size_t i = 0; i < 4; i++)
		ref[i].color = mColor;
}

sf::Vertex * vertex_reference::get_reference()
{
	assert(mBatch != nullptr);
	return &mBatch->mVertices[mIndex];
}

vertex_batch::vertex_batch()
{
	mUse_render_texture = false;
}

void vertex_batch::set_texture(std::shared_ptr<texture> pTexture)
{
	mTexture = pTexture;
}

vertex_reference vertex_batch::add_quad(fvector pPosition, frect pTexture_rect, int pRotation)
{
	mVertices.resize(mVertices.size() + 4);

	vertex_reference ref;
	ref.mBatch = this;
	ref.mIndex = mVertices.size() - 4;
	ref.set_texture_rect(pTexture_rect);
	ref.set_rotation(pRotation);
	ref.set_position(pPosition);
	ref.set_color({ 255, 255, 255, 255 });
	return ref;
}

void vertex_batch::reserve_quads(size_t pAmount)
{
	mVertices.reserve(mVertices.size() + pAmount);
}

int vertex_batch::draw(renderer &pR)
{
	if (!mTexture)
		return 1;
	return draw(pR, mTexture->sfml_get_texture());
}

void vertex_batch::use_render_texture(bool pEnable)
{
	mUse_render_texture = pEnable;
}

int vertex_batch::draw(renderer & pR, const sf::Texture & pTexture)
{
	if (mVertices.empty())
		return 1;

	sf::RenderStates rs;
	rs.transform.rotate(get_absolute_rotation());
	rs.transform.scale(get_absolute_scale());
	rs.texture = &pTexture;
	if (mShader)
		rs.shader = mShader->get_sfml_shader();

	if (mUse_render_texture)
	{
		const sf::Vector2f position_nondec = get_exact_position().floor();
		rs.transform.translate(position_nondec + sf::Vector2f(1, 1));

		if (get_renderer() != &pR)
			set_renderer(pR, true);
		mRender.clear({ 0, 0, 0, 0 });
		mRender.draw(&mVertices[0], mVertices.size(), sf::Quads, rs);
		mRender.display();
		sf::Sprite final_render(mRender.getTexture());
		final_render.setPosition((get_exact_position() - position_nondec)
			- sf::Vector2f(1, 1));
		pR.get_sfml_render().draw(final_render);
	}
	else
	{
		rs.transform.translate(get_exact_position());
		pR.get_sfml_render().draw(&mVertices[0], mVertices.size(), sf::Quads, rs);
	}
	return 0;
}

void vertex_batch::clean()
{
	mVertices.clear();
}

void vertex_batch::set_color(color pColor)
{
	for (auto& i : mVertices)
	{
		i.color = pColor;
	}
}

void vertex_batch::refresh_renderer(renderer& pR)
{
	if (mUse_render_texture)
	{
		const auto target = pR.get_target_size();
		mRender.create(static_cast<unsigned int>(target.x) + 2, static_cast<unsigned int>(target.y) + 2);
	}
}
