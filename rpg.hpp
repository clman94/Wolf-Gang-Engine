#ifndef RPG_HPP
#define RPG_HPP

#include <vector>

#include "node.hpp"
#include "renderer.hpp"
#include "texture.hpp"
#include "time.hpp"
#include "tinyxml2\tinyxml2.h"
#include <list>
#include "rpg_entity.hpp"
#include "rpg_jobs.hpp"
#include "audio.hpp"
#include "rpg_scene.hpp"
#include "rpg_config.hpp"
#include "dictionary.hpp"
#include <set>
#include <map>
#include "utility.hpp"
#include <deque>


namespace rpg
{
class texture_manager
{
	struct texture_entry
	{
		std::string name, path, atlas;
		bool is_loaded, has_atlas;
		engine::texture tex;
	};
	std::list<texture_entry> texturebank;
	texture_entry* find_entry(std::string name);

public:
	int load_settings(std::string path);
	engine::texture* get_texture(std::string name);
};

class panning_node :
	public engine::node
{
	engine::fvector size, viewport;
public:
	void set_bounds_size(engine::fvector s)
	{
		size = s;
	}

	void set_viewport_size(engine::fvector s)
	{
		viewport = s;
	}

	void update_origin(engine::fvector pos)
	{
		engine::fvector npos = pos - (viewport*0.5f);

		if (size.x < viewport.x)
			npos.x = size.x*0.5f - (viewport.x*0.5f);
		else
		{
			if (npos.x < 0) npos.x = 0;
			if (npos.x + viewport.x > size.x) npos.x = size.x - viewport.x;
		}

		if (size.y < viewport.y)
			npos.y = size.y*0.5f - (viewport.y*0.5f);
		else
		{
			if (npos.y < 0) npos.y = 0;
			if (npos.y + viewport.y > size.y) npos.y = size.y - viewport.y;
		}
		set_position(-npos);
	}
};

class event_tracker
{
	struct entry
	{
		size_t c_job;
		interpretor::job_list* c_event;
		entry(interpretor::job_list* e)
			: c_event(e), c_job(0){}
	};
	typedef std::deque<entry> event_hierarchy;	
	
	/*struct thread_entry   // TO THINK ABOUT
	{
		event_hierarchy events;
		bool job_start;
		thread_entry()
			: job_start(true){}
	};
	typedef std::deque<thread_entry> thread_list;
	thread_list threads;*/

	event_hierarchy events;
	bool job_start;

public:
	event_tracker()
	{
		job_start = true;
	}

	bool is_start()
	{
		return job_start;
	}

	void next_job()
	{
		if (events.size())
		{
			++events.back().c_job;
			job_start = true;
		}
	}

	void wait_job()
	{
		job_start = false;
	}

	interpretor::job_entry* get_job()
	{
		if (!events.size()) return nullptr;
		while (events.back().c_job >= events.back().c_event->size())
		{
			events.pop_back();
			if (!events.size()) return nullptr;
			next_job();
		}
		return events.back().c_event->at(events.back().c_job).get();
	}

	void call_event(interpretor::job_list* e)
	{
		events.emplace_back(e);
		job_start = true;
	}

	void cancel_event()
	{
		events.pop_back();
	}

	void cancel_all()
	{
		events.clear();
	}

	void interrupt(interpretor::job_list* e)
	{
		cancel_all();
		call_event(e);
	}

	void queue_event(interpretor::job_list* e)
	{
		events.emplace_front(e);
	}
};


// I know, I know Its a VERY large class.
class game
{
	std::list<scene> scenes;
	scene* c_scene;
	scene* find_scene(const std::string name);
	void switch_scene(scene* nscene);

	bool check_event_collisionbox(int type, engine::fvector pos);
	bool check_wall_collisionbox(engine::fvector pos, engine::fvector size);

	event_tracker tracker;
	int           tick_interpretor();

	// Contains shadow pair of is_global_entity bool value
	std::list<utility::shadow_pair<entity, bool>> entities;
	entity* find_entity(std::string name);
	void clear_entities();
	utility::error load_entity(std::string path, bool is_global_entity = false);
	utility::error load_entities_list(tinyxml2::XMLElement* e, bool is_global_entity = false);
	utility::error load_entity_anim(tinyxml2::XMLElement* e, entity& c);

	entity *main_character;   // Main character pointer
	bool    lock_mc_movement; // Locks the movement of the main character is true.
	bool    is_mc_moving();   // Simply checks for the directional controls.
	int     mc_movement();    // Calculates the movement and animation of character

	rpg::texture_manager tm;
	panning_node root;

	struct{
		engine::rectangle_node fade_overlap;
		engine::rectangle_node narrow_focus[2];
	} graphic_fx;

	struct{
		engine::sprite_node box;
		engine::sprite_node cursor;
		engine::text_node   text;
		engine::text_node   option1;
		engine::text_node   option2;
		entity             *speaker;
		engine::render_client_special
			<engine::animated_sprite_node> expression;
	} narrative;
	void open_narrative_box();
	void close_narrative_box();

	struct{
		engine::tile_bind_node misc;
		engine::tile_node ground;
	}tile_system;

	engine::font font;
	engine::renderer* renderer;
	
	// The map of expression animations all identified by a string.
	std::map<std::string, engine::animated_sprite_node> expressions;

	// The set of flags. All defined by a string.
	// These define the flow of the gam and various other things.
	// They are very important and are recorded in the game save.
	std::set<std::string> flags;
	bool has_flag(std::string name);

	// The control is simply a bool array that 
	// tracks what control (not the key pressed)
	// is activated.
	// Could have used std::set but it simply to heavy for this.
	bool control[CONTROL_COUNT];
	void reset_control(); // Resets all controls to false.

	struct{
		struct{ // Structures inside structures... because organization
			engine::sound_buffer dialog_click_buf;
		}buffers;

		// The sound stream for the background music.
		// Its shadow pair is the path of the current file.
		utility::shadow_pair<engine::sound_stream, std::string> bg_music;

		engine::sound FX_dialog_click;
		engine::sound_spawner spawner;
	} sound;

	// Convenience function
	void set_text_default(engine::text_node& n);

	engine::clock frame_clock;

	int load_tilemap(tinyxml2::XMLElement* e, size_t layer = 0);
	int load_tilemap_individual(tinyxml2::XMLElement* e, size_t layer = 0);
	void clean_scene();
public:
	enum control_type
	{
		LEFT,
		RIGHT,
		UP,
		DOWN,
		SELECT_PREV,
		SELECT_NEXT,
		ACTIVATE,
		EXIT
	};

	game();
	void trigger_control(control_type key);
	
	int set_maincharacter(std::string name);
	void set_renderer(engine::renderer& r);
	utility::error load_textures(std::string path);
	utility::error load_scene(std::string path);
	utility::error load_game(std::string path);
	interpretor::job_list* find_event(std::string name);
	int tick(engine::renderer& _r);
	engine::node& get_root();
	texture_manager& get_texture_manager();
};

}

#endif