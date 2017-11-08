#include <rpg/sprite_entity.hpp>

using namespace rpg;

sprite_entity::sprite_entity()
{
	set_dynamic_depth(true);
	mSprite.set_anchor(engine::anchor::bottom);
	mSprite.set_internal_parent(*this);
}


int sprite_entity::draw(engine::renderer &pR)
{
	update_depth();
	mSprite.set_unit(get_unit());
	mSprite.set_position(calculate_offset());
	mSprite.draw(pR);
	return 0;
}