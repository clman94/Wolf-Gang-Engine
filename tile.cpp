#define ENGINE_INTERNAL

#include "renderer.hpp"

using namespace engine;

tile_node::tile_node()
{
}

void
tile_node::set_texture(texture& tex)
{
	mTexture = &tex;
}

int
tile_node::find_tile(fvector pPosition, size_t pLayer)
{
	for (size_t i = 0; i < mTiles.size(); i++)
	{
		if (mTiles[i].layer == pLayer &&
			mTiles[i].pos.x == pPosition.x &&
			mTiles[i].pos.y == pPosition.y)
			return mTiles[i].index;
	}
	return -1;
}


void
tile_node::set_tile_size(fvector s)
{
	mTile_size = s;
}

void
tile_node::set_tile(fvector pos, std::string atlas, size_t layer, int rot, bool replace)
{
	auto crop = mTexture->get_entry(atlas);

	int width = mTile_size.x;
	int height = mTile_size.x;

	sf::Vertex v[4];
	v[(rot    ) % 4].texCoords = sf::Vector2f(crop.x              , crop.y);
	v[(rot + 1) % 4].texCoords = sf::Vector2f(crop.x + crop.w     , crop.y);
	v[(rot + 2) % 4].texCoords = sf::Vector2f(crop.x + crop.w     , crop.y + crop.h);
	v[(rot + 3) % 4].texCoords = sf::Vector2f(crop.x              , crop.y + crop.h);
	v[0].position              = sf::Vector2f(pos.x*width         , pos.y*height);
	v[1].position              = sf::Vector2f(pos.x*width + crop.w, pos.y*height);
	v[2].position              = sf::Vector2f(pos.x*width + crop.w, pos.y*height + crop.h);
	v[3].position              = sf::Vector2f(pos.x*width         , pos.y*height + crop.h);

	int index = find_tile(pos, layer);
	if (index == -1)
	{
		auto &vertices = *mLayers[layer];
		mTiles.emplace_back(pos, vertices.size(), layer);
		vertices.push_back(v[0]);
		vertices.push_back(v[1]);
		vertices.push_back(v[2]);
		vertices.push_back(v[3]);
	}
	else if (replace)
	{
		auto &vertices = *mLayers[layer];
		vertices[index]     = v[0];
		vertices[index + 1] = v[1];
		vertices[index + 2] = v[2];
		vertices[index + 3] = v[3];
	}
}

void tile_node::remove_tile(fvector pos, size_t layer)
{
	int i = find_tile(pos, layer);
}

int
tile_node::draw(renderer &_r)
{
	if (!mLayers.size()) return 1;

	sf::RenderStates rs;
	auto pos = get_exact_position();
	rs.transform.translate({ std::floor(pos.x), std::floor(pos.y) }); // floor prevents those nasty lines
	rs.texture = &mTexture->sfml_get_texture();

	for (auto &i : mLayers)
	{
		auto &vertices = *i.second;
		_r.mWindow.draw(&vertices[0], vertices.size(), sf::Quads, rs);
	}
	return 0;
}

void
tile_node::clear_all()
{
	mLayers.clear();
	mTiles.clear();
}