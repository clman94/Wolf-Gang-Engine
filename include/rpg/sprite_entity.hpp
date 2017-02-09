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
	void play_animation();
	void stop_animation();
	void tick_animation();
	bool set_animation(const std::string& pName, bool pSwap = false);
	int draw(engine::renderer &pR);
	void set_texture(std::shared_ptr<engine::texture> pTexture);
	void set_anchor(engine::anchor pAnchor);
	void set_color(engine::color pColor);
	void set_rotation(float pRotation);
	void set_shader(std::shared_ptr<engine::shader> pShader);

	engine::fvector get_size() const;
	bool is_playing() const;

	virtual entity_type get_entity_type()
	{
		return entity_type::sprite;
	}

private:
	virtual void update_z();

private:
	engine::animation_node mSprite;
};

}
#endif // !RPG_SPRITE_ENTITY_HPP
