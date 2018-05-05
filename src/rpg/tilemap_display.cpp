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

bool tilemap_display::add_tile(engine::fvector pPosition, engine::subtexture::ptr pAtlas, std::size_t pLayer, int pRotation)
{
	assert(mTexture != nullptr);

	// Go to the layer
	auto layer_iter = mLayers.begin();
	std::advance(layer_iter, pLayer);

	// Add quad to vertex batch
	auto hnd = layer_iter->add_quad_texture(mTexture, pPosition*get_unit(), pAtlas->get_root_frame(), {1, 1, 1, 1}, pRotation);
	
	// Register animated tile
	if (pAtlas->get_frame_count() > 1)
	{
		animated_tile n_anim_tile;
		n_anim_tile.layer = pLayer;
		n_anim_tile.hnd = hnd;
		n_anim_tile.set_animation(pAtlas);
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

		//i.use_render_texture(true);

		i.draw(pR);
	}
	return 0;
}

void tilemap_display::update_animations()
{
	try {
		for (auto& i : mAnimated_tiles)
		{
			if (i.update_animation())
			{
				auto layer_iter = mLayers.begin();
				std::advance(layer_iter, i.layer);
				layer_iter->change_texture_rect(i.hnd, i.get_current_frame());
			}
		}
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

void tilemap_display::update(const tilemap_manipulator& pTile_manipulator)
{
	clear();

	mTexture = pTile_manipulator.get_texture();

	for (size_t i = 0; i < pTile_manipulator.get_layer_count(); i++)
	{
		mLayers.emplace_back();
		const tilemap_layer& layer = pTile_manipulator.get_layer(i);
		for (size_t j = 0; j < layer.get_tile_count(); j++)
		{
			const tile& t = *layer.get_tile(j);
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

bool tilemap_display::animated_tile::update_animation()
{
	if (!mAnimation) return false;
	if (!mAnimation->get_frame_count()) return false;
	if (mTimer.is_reached())
	{
		++mFrame;
		mTimer.start(mAnimation->get_interval(mFrame + mAnimation->get_default_frame())*0.001f);
		return true;
	}
	return false;
}

engine::frect tilemap_display::animated_tile::get_current_frame() const
{
	return mAnimation->get_frame_at(mFrame + mAnimation->get_default_frame());
}
