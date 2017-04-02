#include <rpg/sprite_entity.hpp>

using namespace rpg;

sprite_entity::sprite_entity()
{
	set_dynamic_depth(true);
	mSprite.set_anchor(engine::anchor::bottom);
}


int sprite_entity::draw(engine::renderer &pR)
{
	update_depth();
	mSprite.set_unit(get_unit());
	mSprite.set_position(get_absolute_position() - engine::fvector(0, get_z()));
	mSprite.draw(pR);
	return 0;
}