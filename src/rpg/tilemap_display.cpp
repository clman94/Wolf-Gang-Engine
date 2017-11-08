#include <rpg/tilemap_display.hpp>
#include <engine/log.hpp>

using namespace rpg;

void tilemap_display::set_texture(std::shared_ptr<engine::texture> pTexture)
{
	mTexture = pTexture;
}

std::shared_ptr<engine::texture> tilemap_display::get_texture()
{
	return mTexture;
}

bool tilemap_display::set_tile(engine::fvector pPosition, const std::string& pAtlas, int pLayer, int pRotation)
{
	assert(mTexture != nullptr);

	auto animation = mTexture->get_entry(pAtlas);
	if (!animation)
		return false;

	auto &ntile = mLayers[pLayer].tiles[pPosition];

	ntile.mRef = mLayers[pLayer].vertices.add_quad(pPosition*get_unit()
		, animation->get_frame_at(0)
		, pRotation);
	ntile.set_animation(animation);
	if (animation->get_frame_count() > 1)
		mAnimated_tiles.push_back(&ntile);

	return true;
}

int tilemap_display::draw(engine::renderer& pR)
{
	if (!mTexture) return 1;
	update_animations();
	for (auto &i : mLayers)
	{
		auto& vb = i.second.vertices;

		// Ensure it is a child of this object
		if (!vb.get_parent())
			add_child(vb);
		
		vb.set_texture(mTexture);
		vb.use_render_texture(true);

		// This solves the problem with the lines between the tiles (mostly; still has a few lines)
		//const engine::fvector floating_point_error(11.f / 1024, 11.f / 1024);
		//vb.set_position(get_exact_position().floor() + floating_point_error);
		vb.draw(pR);
	}
	return 0;
}

void tilemap_display::update_animations()
{
	try {
		for (auto i : mAnimated_tiles)
		{
			i->update_animation();
		}
	}
	catch (const std::exception& e)
	{
		logger::error("Exception '" + std::string(e.what()) + "'");
		logger::error("Failed to animate tiles");
		mAnimated_tiles.clear();
	}
}

void tilemap_display::clean()
{
	mLayers.clear();
	mAnimated_tiles.clear();
}

void tilemap_display::set_color(engine::color pColor)
{
	for (auto& l : mLayers)
	{
		l.second.vertices.set_color(pColor);
	}
}

void tilemap_display::highlight_layer(int pLayer, engine::color pHighlight, engine::color pOthers)
{
	for (auto& l : mLayers)
	{
		if (l.first == pLayer)
			l.second.vertices.set_color(pHighlight);
		else
			l.second.vertices.set_color(pOthers);
	}
}

void tilemap_display::remove_highlight()
{
	for (auto& l : mLayers)
	{
		l.second.vertices.set_color({ 255, 255, 255, 255 });
	}
}

void tilemap_display::displayed_tile::set_animation(std::shared_ptr<const engine::animation> pAnimation)
{
	mAnimation = pAnimation;
	mFrame = 0;

	if (pAnimation->get_frame_count() > 0) // Start timer if this is an animation
		mTimer.start(pAnimation->get_interval()*0.001f);
}

void tilemap_display::displayed_tile::update_animation()
{
	if (!mAnimation) return;
	if (!mAnimation->get_frame_count()) return;
	if (mTimer.is_reached())
	{
		++mFrame;
		const size_t rendered_frame = mFrame + mAnimation->get_default_frame();
		mTimer.start(mAnimation->get_interval(rendered_frame)*0.001f);
		mRef.set_texture_rect(mAnimation->get_frame_at(rendered_frame));
	}
}
