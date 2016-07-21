#ifndef RPG_JOBS_HPP
#define RPG_JOBS_HPP

#include <string>
#include <deque>
#include "time.hpp"
#include "tinyxml2\tinyxml2.h"
#include "rpg_entity.hpp"

namespace rpg
{
namespace interpretor
{

enum job_op
{
	INVALID,
	SAY,
	WAIT,
	WAITFORKEY,
	HIDEBOX,
	SELECTION,
	FLAG_SET,
	FLAG_UNSET,
	FLAG_IF,
	FLAG_EXITIF,
	FLAG_ONCE,
	SCENE_LOAD,
	TILE_REPLACE,
	MUSIC_PAUSE,
	MUSIC_SET,
	MUSIC_VOLUME,
	MUSIC_PLAY,
	MUSIC_STOP,
	MUSIC_WAIT,
	ENTITY_MOVE,
	ENTITY_CURRENT,
	ENTITY_SETCYCLEGROUP,
	ENTITY_SETANIMATION,
	ENTITY_SPAWN,
	ENTITY_ANIMATIONSTART,
	ENTITY_ANIMATIONSTOP,
	ENTITY_SETDIRECTION,
	FX_FADEIN,
	FX_FADEOUT,
	FX_SOUND
};

struct job_entry
{
	job_op op;
	job_entry(job_op _op = INVALID)
		: op(_op){}
};

typedef std::vector<engine::ptr_GC<job_entry>> job_list;

struct JOB_say : public job_entry
{
	std::string character, expression, text;
	bool append;
	engine::clock clock;
	size_t c_char;
	engine::time_t char_interval;
	JOB_say(tinyxml2::XMLElement* e, bool _a = false, bool nl = false);
};

struct JOB_wait : public job_entry
{
	int ms;
	engine::clock clock;
	JOB_wait(tinyxml2::XMLElement* e);
};

struct JOB_selection : public job_entry
{
	std::string opt1, opt2, event[2];
	int sel;
	JOB_selection(tinyxml2::XMLElement* e);
};

struct JOB_entity_current : public job_entry
{
	std::string name;
	JOB_entity_current(tinyxml2::XMLElement* e);
};

struct JOB_entity_move : public job_entry
{
	std::string anim, t_character_name;
	entity* t_character;
	engine::clock clock;
	engine::fvector move, norm;
	float speed;
	bool set;

	engine::fvector calculate()
	{
		float delta = (float)1 / clock.restart().s();
		return{ speed*delta, speed*delta };
	}

	JOB_entity_move(tinyxml2::XMLElement* e);
};

struct JOB_flag_set : public job_entry
{
	std::string name;
	JOB_flag_set(tinyxml2::XMLElement* e);
};

struct JOB_flag_unset : public job_entry
{
	std::string name;
	JOB_flag_unset(tinyxml2::XMLElement* e);
};

struct JOB_flag_if : public job_entry
{
	std::string name, event;
	job_list inline_event;
	bool remove;
	JOB_flag_if(tinyxml2::XMLElement* e);
};

struct JOB_flag_exitif : public job_entry
{
	std::string name;
	JOB_flag_exitif(tinyxml2::XMLElement* e);
};

struct JOB_flag_once : public job_entry
{
	std::string name;
	JOB_flag_once(tinyxml2::XMLElement* e);
};


struct JOB_music_set : public job_entry
{
	bool loop;
	float volume;
	std::string path;
	JOB_music_set(tinyxml2::XMLElement* e);
};

struct JOB_music_volume : public job_entry
{
	float volume;
	JOB_music_volume(tinyxml2::XMLElement* e);
};

struct JOB_music_wait : public job_entry
{
	float until_sec;
	JOB_music_wait(tinyxml2::XMLElement* e);
};

struct JOB_scene_load : public job_entry
{
	std::string path;
	JOB_scene_load(tinyxml2::XMLElement* e);
};

struct JOB_tile_replace : public job_entry
{
	std::string name;
	bool is_wall;
	int rot, layer;
	engine::ivector pos1, pos2;
	JOB_tile_replace(tinyxml2::XMLElement* e);
};

struct JOB_entity_setcyclegroup : public job_entry
{
	std::string group_name;
	JOB_entity_setcyclegroup(tinyxml2::XMLElement* e);
};

struct JOB_entity_setanimation : public job_entry
{
	std::string name;
	JOB_entity_setanimation(tinyxml2::XMLElement* e);
};

struct JOB_entity_spawn : public job_entry
{
	std::string as, path;
	JOB_entity_spawn(tinyxml2::XMLElement* e);
};

struct JOB_entity_animationstart : public job_entry
{
	JOB_entity_animationstart(tinyxml2::XMLElement* e);
};

struct JOB_entity_setdirection : public job_entry
{
	int direction;
	JOB_entity_setdirection(tinyxml2::XMLElement* e);
};

struct JOB_fx_fade : public job_entry
{
	engine::clock clock;
	JOB_fx_fade(job_op _op = INVALID){ op = _op; }
};

struct JOB_fx_sound : public job_entry
{
	std::string name;
	float volume;
	JOB_fx_sound(){}
};

job_list parse_jobs_xml(tinyxml2::XMLElement* e);

class event_tracker
{
	struct entry
	{
		size_t c_job;
		job_list* c_event;
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
	event_tracker();
	bool is_start();
	void next_job();
	void wait_job();
	interpretor::job_entry* get_job();
	void call_event(interpretor::job_list* e);
	void cancel_event();
	void cancel_all();
	void interrupt(interpretor::job_list* e);
	void queue_event(interpretor::job_list* e);
};

}
}
#endif