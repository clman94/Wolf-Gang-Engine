#ifndef RPG_INTERPRETER_HPP
#define RPG_INTERPRETER_HPP

#include "time.hpp"
#include "rpg_config.hpp"

#include <string>
#include <vector>
#include <memory>
#include <deque>
#include <cassert>
#include <list>

#include "utility.hpp"
#include "tinyxml2/tinyxml2.h"

namespace rpg{ namespace interpreter{

enum class e_opcode
{
	invalid,
	say,
	wait,
	waitforkey,     //*
	narrative_hide, //*
	selection,
	flag_set,
	flag_unset,
	flag_if,
	flag_exitif,
	flag_once,
	scene_load,
	tile_replace,
	music_pause,  //*
	music_set,
	music_volume,
	music_play,   //*
	music_stop,   //*
	music_wait,
	entity_move,
	entity_current,
	entity_setcyclegroup,
	entity_setanimation,
	entity_spawn,
	entity_animationstart,
	entity_animationstop,
	entity_setdirection,
	fx_fadein,   //*
	fx_fadeout,  //*
	fx_sound
};

class operation
{
	e_opcode op;
public:
	operation() {}
	operation(e_opcode _op)
	{
		op = _op;
	}
	e_opcode get_opcode()
	{
		return op;
	}
	template<class T>
	T* cast_to()
	{
		T* retval = dynamic_cast<T*>(this);
		assert(retval != nullptr);
		return retval;
	}

	virtual int load_xml(tinyxml2::XMLElement* e) { return 1; }
};

class event
{
	std::vector<std::unique_ptr<operation>> ops;
public:
	size_t get_op_count();
	operation* get_op(size_t index);
	operation* create_op(e_opcode op);
	int load_xml_event(tinyxml2::XMLElement* e);
};

class event_tracker
{
	struct event_entry
	{
		size_t c_job;
		event* c_event;
		event_entry(event* e)
			: c_event(e), c_job(0) {}
	};
	typedef std::deque<event_entry> call_stack;

	call_stack events;
	bool job_start;

public:
	event_tracker();
	bool is_start();
	void next();
	void wait();
	void wait_until(bool cond);
	operation* get_current_op();
	void call_event(event* e);
	void cancel_event();
	void cancel_all();
	void interrupt(event* e);
	void queue_event(event* e);
};

class scene_events
{
	struct entry
	{
		std::string name;
		event event;
	};
	std::list<entry> events;
	event_tracker tracker;
	util::error load_event(tinyxml2::XMLElement *e);
public:
	void clear();
	int trigger_event(std::string name);
	event* find_event(std::string name);
	event_tracker& get_tracker();

	util::error load_scene_events(tinyxml2::XMLElement *e);
};


template<e_opcode _OP>
class OP : public operation
{
public:
	OP() : operation(_OP){}
};

/* TEMPLATE
struct OP_ : public OP<e_opcode::>
{
	int load_xml(tinyxml2::XMLElement* e);
};
*/

struct OP_say : public OP<e_opcode::say>
{
	std::string character, expression, text;
	bool append;
	engine::time_t interval;

	OP_say() : append(false){}
	int load_xml(tinyxml2::XMLElement* e);
};

struct OP_wait : public OP<e_opcode::wait>
{
	float seconds;
	engine::clock clock;

	int load_xml(tinyxml2::XMLElement* e);
};

struct OP_flag_set : public OP<e_opcode::flag_set>
{
	std::string flag;
	int load_xml(tinyxml2::XMLElement* e);
};

struct OP_flag_unset : public OP<e_opcode::flag_set>
{
	std::string flag;
	int load_xml(tinyxml2::XMLElement* e);
};

struct entity_action
{
	enum class entity_type
	{
		noncharacter,
		character,
		player
	};
	entity_type type;
	std::string entity_name;

	void load_entity_name(tinyxml2::XMLElement* e);
};

struct OP_entity_move :
	public entity_action,
	public OP<e_opcode::entity_move>
{
	engine::fvector to;
	int load_xml(tinyxml2::XMLElement* e);
};

struct OP_entity_setanimation :
	public entity_action,
	public OP<e_opcode::entity_setanimation>
{
	std::string name;
	int load_xml(tinyxml2::XMLElement* e);
};

struct OP_entity_setcyclegroup :
	public entity_action,
	public OP<e_opcode::entity_setcyclegroup>
{
	std::string name;
	int load_xml(tinyxml2::XMLElement* e);
};

struct OP_entity_setcycle :
	public entity_action,
	public OP<e_opcode::entity_setdirection>
{
	std::string name;
	int load_xml(tinyxml2::XMLElement* e);
};

struct OP_scene_load : public OP<e_opcode::scene_load>
{
	std::string path, door;
	int load_xml(tinyxml2::XMLElement* e);
};


}}


#endif