#include <rpg/tilemap_display.hpp>
#include <rpg/tilemap_manipulator.hpp>

#include <engine/logger.hpp>
#include <cassert>

using namespace rpg;

void tilemap_display::set_texture(std::shared_ptr<engine::texture> pTexture)
{
	mTexture = pTexture;
}

std::shared_ptr<engine::texture> tilemap_display::get_texture()
{
	return mTexture;
}

bool tilemap_display::add_tile(engine::fvector pPosition, const std::string& pAtlas, int pLayer, int pRotation)
{
	assert(mTexture != nullptr);

	auto animation = mTexture->get_entry(pAtlas);
	if (!animation)
		return false;

	// Go to the layer
	auto layer_iter = mLayers.begin();
	std::advance(layer_iter, pLayer);

	// Add quad to vertex batch
	auto ntile = layer_iter->add_quad(pPosition*get_unit()
		, animation->get_frame_at(0)
		, pRotation);

	// Register animated tile
	if (animation->get_frame_count() > 1)
	{
		animated_tile n_anim_tile;
		n_anim_tile.mRef = ntile;
		n_anim_tile.set_animation(animation);
		mAnimated_tiles.push_back(n_anim_tile);
	}
	return true;
}

int tilemap_display::draw(engine::renderer& pR)
{
	if (!mTexture) return 1;
	update_animations();
	for (auto &i : mLayers)
	{
		if (!i.is_visible())
			continue;

		// Ensure it is a child of this object
		if (!i.get_parent())
		{
			i.set_unit(get_unit());
			i.set_internal_parent(*this);
		}
		
		i.set_texture(mTexture);
		//i.use_render_texture(true);

		i.draw(pR);
	}
	return 0;
}

void tilemap_display::update_animations()
{
	try {
		for (auto& i : mAnimated_tiles)
			i.update_animation();
	}
	catch (const std::exception& e)
	{
		logger::error("Exception '" + std::string(e.what()) + "'");
		logger::error("Failed to animate tiles");
		mAnimated_tiles.clear();
	}
}

void tilemap_display::clear()
{
	mLayers.clear();
	mAnimated_tiles.clear();
}

void tilemap_display::highlight_layer(size_t pLayer, engine::color pHighlight, engine::color pOthers)
{
	size_t index = 0;
	for (auto& i : mLayers)
	{
		if (index == pLayer)
			i.set_color(pHighlight);
		else
			i.set_color(pOthers);
		++index;
	}
}

void tilemap_display::remove_highlight()
{
	for (auto& l : mLayers)
		l.set_color({ 255, 255, 255, 255 });
}

void tilemap_display::update(tilemap_manipulator& pTile_manipulator)
{
	clear();
	for (size_t i = 0; i < pTile_manipulator.get_layer_count(); i++)
	{
		mLayers.emplace_back();
		tilemap_layer& layer = pTile_manipulator.get_layer(i);
		layer.explode();
		for (size_t j = 0; j < layer.get_tile_count(); j++)
		{
			tile& t = *layer.get_tile(j);
			add_tile(t.get_position(), t.get_atlas(), i, t.get_rotation());
		}
	}
}

void tilemap_display::set_layer_visible(size_t pIndex, bool pIs_visible)
{
	assert(pIndex < mLayers.size());
	std::next(mLayers.begin(), pIndex)->set_visible(pIs_visible);
}

bool tilemap_display::is_layer_visible(size_t pIndex)
{
	assert(pIndex < mLayers.size());
	return std::next(mLayers.begin(), pIndex)->is_visible();
}

void tilemap_display::animated_tile::set_animation(std::shared_ptr<const engine::animation> pAnimation)
{
	mAnimation = pAnimation;
	mFrame = 0;

	if (pAnimation->get_frame_count() > 0) // Start timer if this is an animation
		mTimer.start(pAnimation->get_interval()*0.001f);
}

void tilemap_display::animated_tile::update_animation()
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
