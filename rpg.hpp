#ifndef RPG_HPP
#define RPG_HPP

#include "renderer.hpp"
#include "utility.hpp"
#include "rpg_managers.hpp"
#include "rpg_config.hpp"
#include "your_soul.hpp"
#include "tinyxml2\tinyxml2.h"

#include <set>
#include <list>
#include <string>
#include <array>
#include <functional>
#include <map>

#include <angelscript.h> // AS_USE_NAMESPACE will need to be defined
#include <angelscript/add_on/contextmgr/contextmgr.h>
#include <angelscript/add_on/scriptbuilder/scriptbuilder.h>

namespace AS = AngelScript;

namespace rpg{

class script_system;

class panning_node :
	public engine::node
{
public:
	void set_boundary(engine::fvector a);
	void set_viewport(engine::fvector a);
	void set_focus(engine::fvector pos);

private:
	engine::fvector boundary, viewport;
};

template<typename T>
class node_list :
	public engine::node
{
public:

	void clear()
	{
		items.clear();
	}

	template<class... T_ARG>
	auto& add_item(T_ARG&&... arg)
	{
		items.emplace_back(std::forward<T_ARG>(arg)...);
		add_child(items.back());
		return items.back();
	}

	auto& add_item()
	{
		items.emplace_back();
		add_child(items.back());
		return items.back();
	}

	auto begin() { return items.begin(); }
	auto end()   { return items.end();   }
	auto back()  { return items.back();  }

private:
	std::list<T> items;
};

class flag_container
{
public:
	bool set_flag(const std::string& name);
	bool unset_flag(const std::string& name);
	bool has_flag(const std::string& name);
	void load_script_interface(script_system& script);

private:
	std::set<std::string> flags;
};

class controls
{
public:
	enum class control
	{
		activate,
		left,
		right,
		up,
		down,
		select_next,
		select_previous,
		reset
	};
	controls();
	void trigger(control c);
	bool is_triggered(control c);
	void reset();

private:
	std::array<bool, 8> c_controls;
};

class entity :
	public engine::render_client,
	public engine::node,
	public   util::named
{
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
	util::error load_entity_xml(std::string path, texture_manager& tm);
	void set_dynamic_depth(bool a);

protected:
	util::error load_animations(tinyxml2::XMLElement* e, texture_manager& tm);
	util::error load_single_animation(tinyxml2::XMLElement* ele, engine::animation &anim, texture_manager& tm);

private:
	struct entity_animation :
		public util::named
	{
		engine::animation anim;
		int type;
	};

	engine::animation_node node;
	std::list<entity_animation> animations;
	entity_animation *c_animation;

	bool dynamic_depth;
};

class character :
	public entity
{
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

private:
	std::string cyclegroup;
	std::string cycle;
	float move_speed;
};

class script_function
{
public:
	script_function();
	~script_function();
	bool is_running();
	void set_engine(AS::asIScriptEngine * e);
	void set_function(AS::asIScriptFunction * f);
	void set_context_manager(AS::CContextMgr * cm);
	void add_arg_obj(unsigned int index, void* ptr);
	void call();

private:
	AS::asIScriptEngine *as_engine;
	AS::asIScriptFunction *func;
	AS::CContextMgr *ctx;
	AS::asIScriptContext *func_ctx;
};

struct collision_box
	: engine::frect
{
public:
	collision_box();
	bool is_valid();
	void validate(flag_container & flags);
	void load_xml(tinyxml2::XMLElement* e);

protected:
	std::string invalid_on_flag;
	std::string spawn_flag;
	bool valid;
};

struct trigger : public collision_box
{
public:
	script_function& get_function();
	void parse_function_metadata(const std::string& metadata);

private:
	script_function func;
};

struct door : public collision_box
{
	std::string name;
	std::string scene_path;
	std::string destination;
};

class collision_system
{
public:
	collision_box* wall_collision(const engine::frect& r);
	door*          door_collision(const engine::fvector& r);
	trigger*       trigger_collision(const engine::fvector& pos);
	trigger*       button_collision(const engine::fvector& pos);

	engine::fvector get_door_entry(std::string name);

	void validate_all(flag_container& flags);

	void add_wall(engine::frect r);
	void add_trigger(trigger& t);
	void add_button(trigger& t);
	void clear();
	util::error load_collision_boxes(tinyxml2::XMLElement* e, flag_container& flags);

private:
	std::list<collision_box> walls;
	std::list<door> doors;
	std::list<trigger> triggers;
	std::list<trigger> buttons;
};

// Excuse this mess -_-
class script_system
{
private:
	AS::asIScriptEngine* as_engine;
	AS::CContextMgr ctxmgr;
	AS::asIScriptModule *scene_module;
	AS::CScriptBuilder builder;

	engine::timer main_timer;

	template<typename T>
	static void as_default_constr(void *memory)
	{
		new(memory) T();
	}

	template<typename T>
	static void as_default_destr(void *memory)
	{
		((T*)memory)->~T();
	}

	void dprint(std::string &msg);
	void register_vector_type();
	void message_callback(const AS::asSMessageInfo * msg);
	std::string get_metadata_type(const std::string &str);
	void script_abort();

public:
	script_system();
	~script_system();
	util::error load_scene_script(const std::string& path);
	void add_function(const char* decl, const AS::asSFuncPtr & ptr, void* instance);
	void add_function(const char* decl, const AS::asSFuncPtr & ptr);
	void add_pointer_type(const char* name);
	void call_event_function(const std::string& name);
	void setup_triggers(collision_system& collision);
	int tick();
};

class narrative_dialog :
	public engine::render_client
{
	engine::sprite_node box;
	engine::sprite_node cursor;
	engine::text_node   text;
	engine::text_node   selection;
	engine::font        font;
	engine::clock       timer;

	bool        revealing;
	size_t      c_char;
	std::string full_text;
	float       interval;

	util::error load_box(tinyxml2::XMLElement* e, texture_manager& tm);
	util::error load_font(tinyxml2::XMLElement* e);

public:
	enum class position
	{
		top,
		bottom
	};

	narrative_dialog();

	void set_box_position(position pos);

	bool is_revealing();

	void reveal_text(const std::string& str, bool append = false);
	void instant_text(std::string str, bool append = false);

	void show_box();
	void hide_box();
	bool is_box_open();

	//void set_style_profile(const std::string& path);

	void set_interval(float ms);

	void show_selection();
	void hide_selection();
	void set_selection(const std::string& str);

	util::error load_narrative_xml(tinyxml2::XMLElement* e, texture_manager& tm);

	void load_script_interface(script_system& script);

	int draw(engine::renderer &r);

protected:
	void refresh_renderer(engine::renderer& r);
};

class tilemap_loader :
	public engine::render_client,
	public engine::node
{
private:
	engine::tile_node node;

	struct tile
	{
		engine::fvector pos, fill;
		int rotation;
		std::string atlas;
		bool collision;
		void load_xml(tinyxml2::XMLElement *e, size_t layer);
		bool is_adjacent_above(tile& a);
		bool is_adjacent_right(tile& a);
	};
	std::map<size_t,std::vector<tile>> tiles;
	tile* find_tile(engine::fvector pos, size_t layer);

	void condense_layer(std::vector<tile> &map);
	util::error load_layer(tinyxml2::XMLElement *e, size_t layer);
	
	tile* find_tile_at(engine::fvector pos, size_t layer);
public:
	tilemap_loader();

	void condense_tiles();

	void set_texture(engine::texture& t);

	util::error load_tilemap_xml(tinyxml2::XMLElement *root);
	util::error load_tilemap_xml(std::string path);
	
	void break_tile(engine::fvector pos, size_t layer);

	void generate(tinyxml2::XMLDocument& doc, tinyxml2::XMLNode* root);
	void generate(const std::string& path);

	int set_tile(engine::fvector pos, engine::fvector fill, size_t layer, const std::string& atlas, int rot);
	int set_tile(engine::fvector pos, size_t layer, const std::string& atlas, int rot);
	void remove_tile(engine::fvector pos, size_t layer);

	void update_display();

	void clear();

	int draw(engine::renderer &_r);
};

class tilemap :
	public engine::render_client,
	public engine::node
{
public:
	tilemap();
	void set_texture(engine::texture& t);
	void set_tile(const std::string& name, engine::fvector pos, engine::fvector fill, size_t layer, int rot);
	util::error load_tilemap_xml(tinyxml2::XMLElement* e, collision_system& collision);
	void clear();
	void load_script_interface(script_system& script);
	int draw(engine::renderer &_r);

private:
	engine::tile_node node;
	util::error load_tilemap(tinyxml2::XMLElement* e, collision_system& collision, size_t layer);
};

class player_character :
	public character
{
public:
	enum class direction
	{
		other,
		up,
		down,
		left,
		right
	};

	player_character();
	void set_locked(bool l);
	bool is_locked();
	void movement(controls &c, collision_system& collision, float delta);
	engine::fvector get_activation_point(float distance = 16);

private:
	bool locked;
	direction facing_direction;
};

class scene :
	public engine::render_proxy,
	public engine::node
{
public:
	scene();
	collision_system& get_collision_system();

	character* find_character(const std::string& name);
	entity*    find_entity(const std::string& name);

	void clean_scene();
	util::error load_scene_xml(std::string path, script_system& script, flag_container& flags);
	util::error reload_scene(script_system& script, flag_container& flags);

	void load_script_interface(script_system& script);

	void set_texture_manager(texture_manager& ntm);

private:
	tilemap_loader tilemap;
	collision_system collision;
	texture_manager * tm;

	node_list<character> characters;
	node_list<entity> entities;

	std::string c_path;

	util::error load_entities(tinyxml2::XMLElement* e);
	util::error load_characters(tinyxml2::XMLElement* e);

	entity*         script_add_entity(const std::string& path);
	entity*         script_add_character(const std::string& path);
	void            script_set_position(entity* e, const engine::fvector& pos);
	engine::fvector script_get_position(entity* e);
	void            script_set_direction(entity* e, int dir);
	void            script_set_cycle(entity* e, const std::string& name);
	void            script_start_animation(entity* e, int type);
	void            script_stop_animation(entity* e, int type);
	void            script_set_animation(entity* e, const std::string& name);

protected:
	void refresh_renderer(engine::renderer& _r);
};

class background_music
{
public:
	void load_script_interface(script_system& script);

private:
	engine::sound_stream bg_music;
};

class game :
	public engine::render_proxy,
	public util::nocopy
{	
public:
	game();
	util::error load_game_xml(std::string path);
	void tick(controls& con);

protected:
	void refresh_renderer(engine::renderer& r);

private:
	panning_node     root_node;
	scene            game_scene;
	player_character player;
	texture_manager  textures;
	flag_container   flags;
	narrative_dialog narrative;
	background_music bg_music;
	engine::clock    frameclock;
	script_system      script;
	controls         c_controls;

	void player_scene_interact();

	entity* script_get_player();
	void load_script_interface();

	float get_delta();
};

}

#endif