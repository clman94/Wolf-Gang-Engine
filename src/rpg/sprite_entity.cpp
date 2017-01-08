#include <rpg/sprite_entity.hpp>

using namespace rpg;

void sprite_entity::set_texture(std::shared_ptr<engine::texture> pTexture)
{
	mSprite.set_texture(pTexture);
}

void sprite_entity::set_anchor(engine::anchor pAnchor)
{
	mSprite.set_anchor(pAnchor);
}

void sprite_entity::set_color(engine::color pColor)
{
	mSprite.set_color(pColor);
}

void sprite_entity::set_rotation(float pRotation)
{
	mSprite.set_rotation(pRotation);
}

engine::fvector sprite_entity::get_size() const
{
	return mSprite.get_size();
}

bool sprite_entity::is_playing() const
{
	return mSprite.is_playing();
}

sprite_entity::sprite_entity()
{
	set_dynamic_depth(true);
	mSprite.set_anchor(engine::anchor::bottom);
}

void sprite_entity::play_animation()
{
	mSprite.start();
}

void sprite_entity::stop_animation()
{
	mSprite.stop();
}

void sprite_entity::tick_animation()
{
	mSprite.tick();
}

bool sprite_entity::set_animation(const std::string& pName, bool pSwap)
{
	return mSprite.set_animation(pName, pSwap);
}

int sprite_entity::draw(engine::renderer &pR)
{
	update_depth();

	mSprite.set_exact_position(get_exact_position());
	mSprite.draw(pR);
	return 0;
}