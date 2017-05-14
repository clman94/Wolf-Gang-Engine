#ifndef RPG_SPRITE_ENTITY_HPP
#define RPG_SPRITE_ENTITY_HPP

#include <engine/resource.hpp>
#include <engine/texture.hpp>
#include <rpg/entity.hpp>

namespace rpg {

class sprite_entity :
	public entity
{
public:
	sprite_entity();
	int draw(engine::renderer &pR);

	virtual type get_type() const
	{ return type::sprite; }

	engine::animation_node mSprite;
};

}
#endif // !RPG_SPRITE_ENTITY_HPP
