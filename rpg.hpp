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

class panning_node :
	public engine::node
{
	engine::fvector boundary, viewport;
public:
	void set_boundary(engine::fvector a)
	{
		boundary = a;
	}

	void set_viewport(engine::fvector a)
	{
		viewport = a;
	}

	void set_focus(engine::fvector pos)
	{
		engine::fvector screen_offset = viewport * 0.5f;
		engine::fvector npos = pos - screen_offset;

		npos.x = util::clamp(npos.x, 0.f, boundary.x - screen_offset.x);
		npos.y = util::clamp(npos.y, 0.f, boundary.y - screen_offset.y);

		set_position(-npos);
	}
};

class flag_container
{
	std::set<std::string> flags;
public:
	bool set_flag(std::string name);
	bool unset_flag(std::string name);
	bool has_flag(std::string name);
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
	controls();
	void trigger(control c);
	bool is_triggered(control c);
	void reset();
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
		movement,
		constant,
		speech,
		user
	};
	entity();
	void play_withtype(e_type type);
	void stop_withtype(e_type type);
	void tick_withtype(e_type type);
	bool set_animation(std::string name);
	int draw(engine::renderer &_r);
	util::error load_entity(std::string path, texture_manager& tm);

protected:
	util::error load_animations(tinyxml2::XMLElement* e, texture_manager& tm);
	util::error load_xml_animation(tinyxml2::XMLElement* ele, engine::animation &anim, texture_manager& tm);
};

class character :
	public entity
{
	std::string cyclegroup;
	std::string cycle;
	float move_speed;
public:

	enum struct e_cycle
	{
		default,
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

	void  set_speed(float f);
	float get_speed();
};

class narrative
{
	engine::sprite_node box;
	engine::sprite_node cursor;
	engine::text_node   text;
};

class collision_system
{
public:
	struct collision_box
		: engine::frect
	{
		std::string invalid_on_flag;
		std::string spawn_flag;
		bool valid;
	};

	struct trigger : public collision_box
	{
		std::string event;
		interpreter::event inline_event;
	};

	struct door : public collision_box
	{
		std::string name;
		std::string scene_path;
		std::string destination;
	};

	collision_box* wall_collision(const engine::frect& r);
	door*          door_collision(const engine::fvector& r);
	trigger*       trigger_collision(const engine::fvector& pos);
	trigger*       button_collision(const engine::fvector& pos);

	void validate_collisionbox(collision_box& cb, flag_container& flags);
	void validate_all(flag_container& flags);

	void add_wall(engine::frect r);
	void clear();
	util::error load_collision_boxes(tinyxml2::XMLElement* e, flag_container& flags);

private:
	std::list<collision_box> walls;
	std::list<door> doors;
	std::list<trigger> triggers;
	std::list<trigger> buttons;
};

class tilemap :
	public engine::render_client,
	public engine::node
{
	engine::tile_node node;
public:
	tilemap();

	void set_texture(engine::texture& t);
	util::error load_tilemap(tinyxml2::XMLElement* e, collision_system& collision, size_t layer);
	void clear();
	int draw(engine::renderer &_r);
};

class player_character :
	public character
{
	bool locked;
public:
	void set_locked(bool l);
	bool is_locked();
	void movement(controls &c, collision_system& collision, float delta);
	engine::fvector get_activation_point();
};

class scene_events
{
	struct entry
	{
		std::string name;
		interpreter::event event;
	};
	std::list<entry> events;
	interpreter::event_tracker tracker;
public:
	void clear();
	interpreter::event* find_event(std::string name);
	interpreter::event_tracker& get_tracker();
	util::error load_event(tinyxml2::XMLElement *e);
};

class scene :
	public engine::render_proxy
{
	tilemap tilemap;
	collision_system collision;
	scene_events events;

	std::list<character> characters;
	std::list<entity> entities;

public:
	collision_system& get_collision_system();
	character* find_character(std::string name);
	entity* find_entity(std::string name);

	util::error load_scene(std::string path, flag_container& flags, engine::renderer& r, texture_manager& tm);

protected:
	void refresh_renderer(engine::renderer& _r);
};

class game :
	public engine::render_proxy
{
	scene game_scene;
	player_character player;
	texture_manager textures;
	flag_container flags;

	engine::clock frameclock;

public:
	scene& get_scene();
	util::error load_game(std::string path);
	void tick(controls& con);

protected:
	void refresh_renderer(engine::renderer& _r);
};

}

#endif