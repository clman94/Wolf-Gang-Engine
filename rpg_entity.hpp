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
	animation* world_animation[6];
	int c_cycle;
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
		CONSTANT
	};

	entity();
	std::string get_name();
	void set_name(std::string _name);
	utility::error set_cycle_animation(std::string name, cycle_type type);
	utility::error set_cycle_group(std::string name);
	void set_cycle(int cycle);
	engine::fvector get_activate_point();

	engine::animated_sprite_node* get_animation();

	void animation_start(animation_type type)
	{
		if (c_anim && c_anim->type == type)
			c_anim->node.start();
	}

	void animation_stop(animation_type type)
	{
		if (c_anim && c_anim->type == type)
			c_anim->node.stop();
	}

	void move_left(float delta);
	void move_right(float delta);
	void move_up(float delta);
	void move_down(float delta);

	int draw(engine::renderer &_r);
	friend class game;
};


/* I was trying to improve the class above and ended up only rewritting the same thing
class entity
	:public engine::render_client
{
	struct animation
	{
		engine::animated_sprite_node n;
		std::string name;
		int type;
	};
	std::list<animation> animations;
	animation* find_animation(std::string name)
	{
		for (auto& i : animations)
		{
			if (i.name == name)
				return &i;
		}
		return nullptr;
	}
	animation* cycles[5];
	int c_cycle;

	std::string name;
public:

	const std::string& get_name()
	{
		return name;
	}

	int get_entity_type(){ return TYPE_CHARACTER; }

	enum animation_cycle
	{
		LEFT,
		RIGHT,
		UP,
		DOWN,
		MISC
	};

	enum animation_type
	{
		TYPE_WORLD_WALK,
		TYPE_WORLD_SPEECH
	};

	int set_cycle(int cycle)
	{
		c_cycle = cycle;
	}

	int set_cycle_animation(int cycle, std::string name)
	{
		auto anim = find_animation(name);
		if (!anim) return 1;
		cycles[cycle] = anim;
		return 0;
	}

	void move_up(engine::fvector offset)
	{

	}

	// Only relevant to the main character
	engine::fvector get_activate_point()
	{
		using namespace engine;
		auto pos = get_relative_position();
		switch (c_cycle)
		{
		case LEFT:
			return pos - fvector(32, 0);
		case RIGHT:
			return pos + fvector(32, 0);
		case UP:
			return pos - fvector(0, 32);
		case DOWN:
			return pos + fvector(0, 32);
		}
		return pos;
	}

	// game class contains everything that loads this class
	friend class game;
};*/

}

#endif



/*
GOALS

- Render 2 animations; one on the world, one as the expression
- The world animation should be able to "speek" as well with the
  expression if the currnt animation is set too.

*/

