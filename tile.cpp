#define ENGINE_INTERNAL

#include "renderer.hpp"

using namespace engine;

// ################
// tile_sprite_node
// ################

int 
tile_sprite_node::update_tile_render(renderer &_r)
{
	for (auto i : tiles)
	{
		i.sprite->set_depth((int)i.pos.y);
		if (!i.sprite->is_rendered())
			_r.add_client(i.sprite.get());
	}
	return 0;
}

void 
tile_sprite_node::set_tile_size(float w, float h)
{
	width = w;
	height = h;
}

tile_sprite_node::tile_sprite*
tile_sprite_node::create_tile(int x, int y)
{
	tile_sprite* tile = nullptr;
	tiles.push_back(tile_sprite());
	tile = &tiles.back();
	tile->sprite = ptr_GC<sprite_node>(new sprite_node);
	tile->sprite->set_parent(*this);
	tile->sprite->set_position({ x*width, (y + 1)*height/* anchor it on bottom left*/ });
	tile->pos.x = x;
	tile->pos.y = y;
	return tile;
}

int 
tile_sprite_node::set_tile(int x, int y, texture& tex)
{
	tile_sprite* tile = find_tile(x, y);
	if (!tile)
		tile = create_tile(x, y);
	tile->sprite->set_texture(tex);
	tile->sprite->set_anchor(anchor::bottomleft);
	return 0;
}

int
tile_sprite_node::set_tile(int x, int y, texture& tex, std::string atlas)
{
	tile_sprite* tile = find_tile(x, y);
	if (!tile)
		tile = create_tile(x, y);
	tile->sprite->set_texture(tex, atlas);
	tile->sprite->set_anchor(anchor::bottomleft);
	return 0;
}

tile_sprite_node::tile_sprite*
tile_sprite_node::find_tile(int x, int y, size_t& index)
{
	for (index = 0; index < tiles.size(); index++)
	{
		tile_sprite& tile = tiles[index];
		if (tile.pos.x == x && tile.pos.y == y)
		{
			return &tile;
		}
	}
	index = -1;
	return nullptr;
}

tile_sprite_node::tile_sprite*
tile_sprite_node::find_tile(int x, int y)
{
	size_t i = -1;
	return find_tile(x, y, i);
}


int
tile_sprite_node::remove_tile(int x, int y)
{
	size_t index = -1;
	tile_sprite* tile = find_tile(x, y, index);
	if (!tile) return 1;
	tile->sprite->set_visible(false);
	tiles.erase(tiles.begin() + index);
	return 1;
}

ptr_GC<sprite_node> 
tile_sprite_node::get_sprite(int x, int y)
{
	tile_sprite* tile = find_tile(x, y);
	if (!tile) return nullptr;
	return tile->sprite;
}


// ##############
// tile_bind_node
// ##############

void
tile_bind_node::set_tile_size(ivector s)
{
	tile_size = s;
}

size_t
tile_bind_node::find_tile(ivector pos)
{
	for (size_t i = 0; i < tiles.size(); i++)
	{
		tile_entry& tile = tiles[i];
		if (tile.pos.x == pos.x && tile.pos.y == pos.y)
			return i;
	}
	return -1;
}

int
tile_bind_node::bind_tile(ptr_GC<node> n, ivector pos, bool replace)
{
	size_t te = find_tile(pos);
	if (te == -1 && replace)
		unbind_tile(pos);
	n->set_parent(*this);
	n->set_position(tile_size*pos);
	tiles.push_back({ pos, n });
	return 0;
}

ptr_GC<node>
tile_bind_node::unbind_tile(ivector pos)
{
	size_t te = find_tile(pos);
	if (te == -1) return nullptr;
	tile_entry tile = tiles[te];
	tile.node->detach_parent();
	tiles.erase(tiles.begin() + te);
	return tile.node;
}

void
tile_bind_node::clear_all()
{
	tiles.clear();
}

// #########
// tile_node
// #########

tile_node::tile_node()
{
}

void
tile_node::set_texture(texture& tex)
{
	c_tex = &tex;
}

int
tile_node::find_tile(fvector pos, size_t layer)
{
	for (size_t i = 0; i < entries.size(); i++)
	{
		if (entries[i].layer == layer &&
			entries[i].pos.x == pos.x &&
			entries[i].pos.y == pos.y)
			return entries[i].index;
	}
	return -1;
}


void
tile_node::set_tile_size(fvector s)
{
	tile_size = s;
}

void
tile_node::set_tile(fvector pos, std::string atlas, size_t layer, int rot, bool replace)
{
	auto crop = c_tex->get_entry(atlas);

	int width = tile_size.x;
	int height = tile_size.x;

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
		auto &vertices = *layers[layer];
		entries.emplace_back(pos, vertices.size(), layer);
		vertices.push_back(v[0]);
		vertices.push_back(v[1]);
		vertices.push_back(v[2]);
		vertices.push_back(v[3]);
	}
	else if (replace)
	{

		auto &vertices = *layers[layer];
		vertices[index]     = v[0];
		vertices[index + 1] = v[1];
		vertices[index + 2] = v[2];
		vertices[index + 3] = v[3];
	}
}

int
tile_node::draw(renderer &_r)
{
	if (!layers.size()) return 1;

	sf::RenderStates rs;
	auto pos = get_exact_position();
	rs.transform.translate({ std::floor(pos.x), std::floor(pos.y) }); // floor prevents those nasty lines
	rs.texture = &c_tex->sfml_get_texture();

	for (auto &i : layers)
	{
		auto &vertices = *i.second;
		_r.window.draw(&vertices[0], vertices.size(), sf::Quads, rs);
	}
	return 0;
}

void
tile_node::clear_all()
{
	layers.clear();
	entries.clear();
}