#ifndef RPG_HPP
#define RPG_HPP

#include "renderer.hpp"
#include "utility.hpp"
#include "rpg_managers.hpp"
#include "rpg_config.hpp"
#include "your_soul.hpp"
#include "tinyxml2\tinyxml2.h"
#include "rpg_interpreter.hpp"

#include <set>
#include <list>
#include <string>
#include <array>

namespace rpg{

class flag_container
{
	std::set<std::string> flags;
public:
	bool set_flag(std::string name);
	bool unset_flag(std::string name);
	bool has_flag(std::string name);
};

class entity :
	public engine::render_client,
	public engine::node,
	public   util::named
{
	struct entity_animation :
		public util::named
	{
		engine::animation anim;
		int type;
	};

	engine::animation_node node;
	std::list<entity_animation> animations;
	entity_animation *c_animation;

public:
	enum e_type
	{
		constant,
		speech,
		user,
		movement
	};
	entity();
	void play_withtype(e_type type);
	void stop_withtype(e_type type);
	void tick_withtype(e_type type);
	bool set_animation(std::string name);
	int draw(engine::renderer &_r);
	util::error load_animations(tinyxml2::XMLElement* e, texture_manager& tm);
};

class character :
	public entity
{
	std::string cyclegroup;
	std::string cycle;
public:

	enum struct e_cycle
	{
		left,
		right,
		up,
		down,
		idle
	};

	character();
	void set_cycle_group(std::string name);
	void set_cycle(std::string name);
	void set_cycle(e_cycle type);
};

class narrative
{
	engine::sprite_node box;
	engine::sprite_node cursor;
	engine::text_node   text;
};

class tilemap :
	public engine::render_client,
	public engine::node
{
	engine::tile_node node;
public:

	tilemap();
	void set_texture(engine::texture& t);
	util::error load_tilemap(tinyxml2::XMLElement* e, size_t layer);
	void clear();
	int draw(engine::renderer &_r);
};

class collision_system
{
public:
	struct collision_box
		: engine::frect
	{
		std::string invalid_on_flag;
		bool valid;
	};

	struct trigger : public collision_box
	{
		std::string event;
		interpreter::event inline_event;
		std::string spawn_flag;
	};

	struct door : public collision_box
	{
		std::string name;
		std::string scene_path;
		std::string destination;
	};

	collision_box* wall_collision(const engine::frect& r)
	{
		for (auto &i : walls)
		{
			if (i.valid && i.is_intersect(r))
				return &i;
		}
		return nullptr;
	}

	door* door_collision(const engine::frect& r)
	{
		for (auto &i : doors)
		{
			if (i.valid && i.is_intersect(r))
				return &i;
		}
		return nullptr;
	}

	util::error load_collision_boxes(tinyxml2::XMLElement* e, flag_container& flags);

private:
	std::list<collision_box> walls;
	std::list<door> doors;
	std::list<trigger> triggers;
	std::list<trigger> buttons;
};

class player_character :
	public character
{
public:
	engine::fvector get_activation_point();

};

class scene_interpreter
{
	std::list<interpreter::event> events;
public:

};

class scene :
	public engine::render_proxy
{
	tilemap tilemap;
	
	collision_system collision;

	std::list<character> characters;
	std::list<entity> entities;
public:
	character* find_character(std::string name);
	entity* find_entity(std::string name);

	util::error load_scene(std::string path, engine::renderer& r, texture_manager& tm);

protected:
	void refresh_renderer(engine::renderer& _r);
};

class controls
{
	std::array<bool, 7> c_controls;
public:
	enum class control
	{
		activate,
		left,
		right,
		up,
		down,
		select_next,
		select_previous
	};

	void trigger_control(control c);
	bool is_triggered(control c);
	void reset();
};

class game :
	public engine::render_proxy
{
	rpg::scene scene;
	player_character player;
	texture_manager textures;
public:
	util::error load_game(std::string path);
	void tick();

protected:
	void refresh_renderer(engine::renderer& _r);
};

}

#endif