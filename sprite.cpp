#define ENGINE_INTERNAL

#include "renderer.hpp"

using namespace engine;

int sprite_node::draw(renderer &_r)
{
	fvector loc = get_position();
	_sprite.setPosition(sf::Vector2f((float)loc.x, (float)loc.y) + offset);
	_r.window.draw(_sprite);
	return 0;
}

void sprite_node::set_scale(fvector s)
{
	_sprite.setScale(sf::Vector2f(s.x, s.y));
}

int sprite_node::set_texture(texture& tex)
{
	_sprite.setTexture(tex.sfml_get_texture());
	return 0;
}

int sprite_node::set_texture(texture& tex, std::string atlas)
{
	sf::Texture& sf_texture = tex.sfml_get_texture();
	_sprite.setTexture(sf_texture);
	texture_crop crop;
	if (tex.find_atlas(atlas, crop))
		return 1;
	_sprite.setTextureRect(sf::IntRect(crop.x, crop.y, crop.w, crop.h));
	return 0;
}

void 
sprite_node::set_anchor(anchor _anchor)
{
	sf::IntRect rect = _sprite.getTextureRect();

	switch (_anchor)
	{
	case topleft:
		return;
	case bottomleft:
		_sprite.setOrigin(sf::Vector2f(0, (float)rect.height));
	}
}

fvector 
sprite_node::get_size()
{
	sf::IntRect rect = _sprite.getTextureRect();
	return fvector((float)rect.width, (float)rect.height);
}