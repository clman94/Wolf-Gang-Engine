#include <rpg/sprite_entity.hpp>

using namespace rpg;

int sprite_entity::set_texture(std::string pName, texture_manager & pTexture_manager)
{
	auto texture = pTexture_manager.get_texture(pName);
	if (!texture)
	{
		util::error("Cannot find texture for entity");
		return 1;
	}
	mTexture = texture;
	return 0;
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
	if (!mTexture)
		return false;

	auto animation = mTexture->get_animation(pName);
	if (!animation)
		return false;

	mSprite.set_animation(*animation, pSwap);
	return true;
}

int sprite_entity::draw(engine::renderer &_r)
{
	update_depth();

	mSprite.set_exact_position(get_exact_position());
	mSprite.draw(_r);
	return 0;
}