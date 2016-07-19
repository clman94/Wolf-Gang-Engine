#ifndef RPG_SCENE_HPP
#define RPG_SCENE_HPP

#include <list>
#include "rpg_jobs.hpp"
#include "tinyxml2\tinyxml2.h"
#include "utility.hpp"

namespace rpg
{
struct scene
{
	std::string name;

	struct event_entry
	{
		std::string name;
		interpretor::job_list jobs;
		event_entry(std::string _name, interpretor::job_list& _jobs)
			: name(_name), jobs(_jobs){}
	};
	std::list<event_entry> events;
	int parse_events_xml(tinyxml2::XMLElement* e);

	struct collisionbox
	{
		enum box_type
		{
			TOUCH_EVENT,
			WALL,
			BUTTON
		};
		std::string name;
		engine::fvector pos, size;
		int type;
		std::string bind_flag, if_flag;
		interpretor::job_list inline_event;
	};
	std::list<collisionbox> collisionboxes;
	utility::error parse_collisionbox_xml(tinyxml2::XMLElement* e);
};
}

#endif