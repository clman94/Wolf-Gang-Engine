#ifndef RPG_JOBS_HPP
#define RPG_JOBS_HPP

#include <string>
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
	MOVECHARACTER,
	SETCHARACTER,
	SETGLOBAL,
	IFGLOBAL,
	IFGLOBALEXIT,
	SETMUSIC,
	NEWSCENE,
	REPLACETILE,
	PAUSEMUSIC,
	PLAYMUSIC,
	STOPMUSIC,
	SETCYCLEGROUP
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

struct JOB_setcharacter : public job_entry
{
	std::string name;
	JOB_setcharacter(tinyxml2::XMLElement* e);
};

struct JOB_movecharacter : public job_entry
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

	JOB_movecharacter(tinyxml2::XMLElement* e);
};

struct JOB_setglobal : public job_entry
{
	std::string name;
	JOB_setglobal(tinyxml2::XMLElement* e);
};

struct JOB_ifglobal : public job_entry
{
	std::string name, event;
	job_list inline_event;
	bool remove;
	JOB_ifglobal(tinyxml2::XMLElement* e);
};

struct JOB_ifglobalexit : public job_entry
{
	std::string name;
	JOB_ifglobalexit(tinyxml2::XMLElement* e);
};

struct JOB_setmusic : public job_entry
{
	bool loop;
	std::string path;
	JOB_setmusic(tinyxml2::XMLElement* e);
};

struct JOB_newscene : public job_entry
{
	std::string path;
	JOB_newscene(tinyxml2::XMLElement* e);
};

struct JOB_replacetile : public job_entry
{
	std::string name;
	bool is_wall;
	int rot, layer;
	engine::ivector pos1, pos2;
	JOB_replacetile(tinyxml2::XMLElement* e);
};

struct JOB_setcyclegroup : public job_entry
{
	std::string group_name;
	JOB_setcyclegroup(tinyxml2::XMLElement* e);
};

job_list parse_jobs_xml(tinyxml2::XMLElement* e);

}
}
#endif