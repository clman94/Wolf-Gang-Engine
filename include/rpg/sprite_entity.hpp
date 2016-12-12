#ifndef RPG_SPRITE_ENTITY_HPP
#define RPG_SPRITE_ENTITY_HPP

#include <rpg/rpg_managers.hpp>
#include <rpg/entity.hpp>

namespace rpg {

class sprite_entity :
	public entity
{
public:
	sprite_entity();
	void play_animation();
	void stop_animation();
	void tick_animation();
	bool set_animation(const std::string& pName, bool pSwap = false);
	int draw(engine::renderer &pR);
	int set_texture(std::string pName, texture_manager& pTexture_manager);
	void set_anchor(engine::anchor pAnchor);
	void set_color(engine::color pColor);
	void set_rotation(float pRotation);

	engine::fvector get_size() const;
	bool is_playing() const;

	virtual entity_type get_entity_type()
	{
		return entity_type::sprite;
	}

private:
	util::optional_pointer<engine::texture> mTexture;
	engine::animation_node   mSprite;
};

}
#endif // !RPG_SPRITE_ENTITY_HPP
