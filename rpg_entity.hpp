#ifndef RPG_CHARACTER_HPP
#define RPG_CHARACTER_HPP

#include <list>
#include <string>
#include "renderer.hpp"
#include "dictionary.hpp"
#include "utility.hpp"

namespace rpg
{
class entity :
	engine::render_client,
	engine::node
{
	struct animation
	{
		engine::animated_sprite_node node;
		std::string name;
		int type;
	};
	std::list<animation> world;
	animation* find_animation(std::string name);
	animation* c_anim;
	std::string name;
	animation* cycles[6];
	int c_cycle;

	void update_depth();

	float last_y; // for depth automation
	int depth_automation;
public:
	enum cycle_type
	{
		LEFT,
		RIGHT,
		UP,
		DOWN,
		DEFAULT,
		MISC
	};
	enum animation_type
	{
		WALK,
		SPEECH,
		CONSTANT,
		USER_TRIGGERED
	};

	enum depth_auto
	{
		DEPTH_AUTO_NONE,
		DEPTH_AUTO_TO_Y
	};

	entity();
	std::string get_name();
	void set_name(std::string _name);
	utility::error set_cycle_animation(std::string name, cycle_type type);
	utility::error set_cycle_group(std::string name);
	void set_cycle(int cycle);
	engine::fvector get_activate_point();

	void set_auto_depth(int set);

	engine::animated_sprite_node* get_animation();

	bool is_animation_done();
	void animation_start(animation_type type, bool loop = true);
	void animation_stop(animation_type type);

	void move_left(float delta);
	void move_right(float delta);
	void move_up(float delta);
	void move_down(float delta);

	int draw(engine::renderer &_r);
	friend class game;
};

}

#endif
