#include "rpg_jobs.hpp"
#include "rpg.hpp"
#include "rpg_config.hpp"

using namespace engine;
using namespace rpg;
using namespace rpg::interpretor;

interpretor::JOB_say::JOB_say(
	tinyxml2::XMLElement* e,
	bool _a,
	bool nl)
{
	using namespace tinyxml2;

	text = "";
	append = _a;
	if (nl) text += "\n";

	if (auto _character = e->Attribute("char"))
		character = _character;

	if (auto _expression = e->Attribute("expression"))
		expression = _expression;

	if (auto _interval = e->IntAttribute("interval")) 
		char_interval = _interval;
	else char_interval = DEFAULT_DIALOG_SPEED;

	auto text_xml = e->FirstChild();
	while (text_xml)
	{
		if (auto t = text_xml->ToText())
			text += t->Value();
		if (auto ele = text_xml->ToElement())
			text += (std::string(ele->Name()) == "nl" ? "\n" : "");
		text_xml = text_xml->NextSibling();
	}

	op = SAY;
}

interpretor::JOB_wait::JOB_wait(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	ms = (int)(e->FloatAttribute("s") * 1000);
	op = WAIT;
}

interpretor::JOB_selection::JOB_selection(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	sel = 0;

	auto opt1_e = e->FirstChildElement("option1");
	if (!opt1_e) return;

	auto opt2_e = e->FirstChildElement("option2");
	if (!opt2_e) return;

	opt1 = opt1_e->GetText();
	opt2 = opt2_e->GetText();

	event[0] = opt1_e->Attribute("event");
	event[1] = opt2_e->Attribute("event");

	op = SELECTION;
}

interpretor::JOB_entity_current::JOB_entity_current(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	if (auto _name = e->Attribute("name"))
		name = _name;
	op = ENTITY_CURRENT;
}

interpretor::JOB_entity_move::JOB_entity_move(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	move.x = e->FloatAttribute("x");
	move.y = e->FloatAttribute("y");
	speed = e->FloatAttribute("speed");
	if (auto _anim = e->Attribute("animation"))
		anim = _anim;
	if (auto _character = e->Attribute("character"))
		t_character_name = _character;
	set = e->BoolAttribute("set");
	op = ENTITY_MOVE;
}

interpretor::JOB_flag_set::JOB_flag_set(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	if (auto _name = e->Attribute("name"))
		name = _name;
	op = FLAG_SET;
}

interpretor::JOB_flag_unset::JOB_flag_unset(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	if (auto _name = e->Attribute("name"))
		name = _name;
	op = FLAG_UNSET;
}

interpretor::JOB_flag_if::JOB_flag_if(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	if (auto _name = e->Attribute("name"))
		name = _name;
	remove = e->BoolAttribute("remove");
	if (auto _event = e->Attribute("event"))
		event = _event;
	else
		inline_event = parse_jobs_xml(e);
	op = FLAG_IF;
}

interpretor::JOB_flag_exitif::JOB_flag_exitif(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	if (auto _name = e->Attribute("name"))
		name = _name;
	op = FLAG_EXITIF;
}

interpretor::JOB_flag_once::JOB_flag_once(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	if (auto _name = e->Attribute("name"))
		name = _name;
	op = FLAG_ONCE;
}

interpretor::JOB_music_set::JOB_music_set(tinyxml2::XMLElement* e)
{
	if (auto _path = e->Attribute("path"))
		path = _path;
	loop = e->BoolAttribute("loop");
	float vol = e->FloatAttribute("volume");
	volume = vol ? vol : 100; // defualt max volume
	op = MUSIC_SET;
}

interpretor::JOB_music_volume::JOB_music_volume(tinyxml2::XMLElement* e)
{
	volume = e->FloatAttribute("set");
	op = MUSIC_VOLUME;
}

interpretor::JOB_music_wait::JOB_music_wait(tinyxml2::XMLElement* e)
{
	until_sec = e->FloatAttribute("until");
	op = MUSIC_WAIT;
}


interpretor::JOB_scene_load::JOB_scene_load(tinyxml2::XMLElement* e)
{
	if (auto _path = e->Attribute("path"))
		path = _path;
	op = SCENE_LOAD;
}

interpretor::JOB_tile_replace::JOB_tile_replace(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	pos1.x   = e->IntAttribute("x");
	pos1.y   = e->IntAttribute("y");
	pos2.x   = e->IntAttribute("w") + pos1.x;
	pos2.y   = e->IntAttribute("h") + pos1.y;
	is_wall  = e->BoolAttribute("wall");
	rot      = e->IntAttribute("rot");
	layer = e->IntAttribute("layer");
	if (auto _name = e->Attribute("name"))
		name = _name;
	op = TILE_REPLACE;
}

interpretor::JOB_entity_setcyclegroup::JOB_entity_setcyclegroup(tinyxml2::XMLElement* e)
{
	if (auto _name = e->Attribute("name"))
		group_name = _name;
	op = ENTITY_SETCYCLEGROUP;
}

interpretor::JOB_entity_setanimation::JOB_entity_setanimation(tinyxml2::XMLElement* e)
{
	if (auto _name = e->Attribute("name"))
		name = _name;
	op = ENTITY_SETANIMATION;
}

interpretor::JOB_entity_spawn::JOB_entity_spawn(tinyxml2::XMLElement* e)
{
	if (auto _as = e->Attribute("as"))
		as = _as;
	if (auto _path = e->Attribute("path"))
		path = _path;
	op = ENTITY_SPAWN;
}

interpretor::JOB_entity_animationstart::JOB_entity_animationstart(tinyxml2::XMLElement* e)
{
	op = ENTITY_ANIMATIONSTART;
}

interpretor::JOB_entity_setdirection::JOB_entity_setdirection(tinyxml2::XMLElement* e)
{
	if (auto _dir = e->Attribute("direction"))
	{
		if (!strcmp(_dir, "left"))
			direction = entity::LEFT;
		else if (!strcmp(_dir, "right"))
			direction = entity::RIGHT;
		else if (!strcmp(_dir, "up"))
			direction = entity::UP;
		else if (!strcmp(_dir, "down"))
			direction = entity::DOWN;
		else
			utility::error("Invalid direction : Use left, right, up, down");
	}
	else
		utility::error("Please Provide direction attribute");
	op = ENTITY_SETDIRECTION;
}

#define ADD_JOB(A) jobs_ret.push_back(ptr_GC<job_entry>::take(new A))

job_list
interpretor::parse_jobs_xml(tinyxml2::XMLElement* e)
{
	job_list jobs_ret;
	auto j = e->FirstChildElement();

	while (j)
	{
		std::string name = j->Name();
		if (name == "fsay")
			ADD_JOB(JOB_say(j));

		else if (name == "say")
		{
			ADD_JOB(JOB_say(j));
			ADD_JOB(job_entry(WAITFORKEY));
		}

		else if (name == "append")
			ADD_JOB(JOB_say(j, true));

		else if (name == "nl")
			ADD_JOB(JOB_say(j, true, true));

		else if (name == "wait")
			ADD_JOB(JOB_wait(j));

		else if (name == "keywait")
			ADD_JOB(job_entry(WAITFORKEY));

		else if (name == "hidebox")
			ADD_JOB(job_entry(HIDEBOX));

		else if (name == "selection")
			ADD_JOB(JOB_selection(j));

		else if (name == "entity:current")
			ADD_JOB(JOB_entity_current(j));

		else if (name == "entity:move")
			ADD_JOB(JOB_entity_move(j));

		else if (name == "entity:setcyclegroup")
			ADD_JOB(JOB_entity_setcyclegroup(j));

		else if (name == "entity:animation:set")
			ADD_JOB(JOB_entity_setanimation(j));

		else if (name == "entity:animation:start")
			ADD_JOB(JOB_entity_animationstart(j));

		else if (name == "entity:animation:stop")
			ADD_JOB(job_entry(ENTITY_ANIMATIONSTOP));

		else if (name == "entity:setdirection")
			ADD_JOB(JOB_entity_setdirection(j));

		else if (name == "flag:set")
			ADD_JOB(JOB_flag_set(j));

		else if (name == "flag:unset")
			ADD_JOB(JOB_flag_unset(j));

		else if (name == "flag:if")
			ADD_JOB(JOB_flag_if(j));

		else if (name == "flag:exitif")
			ADD_JOB(JOB_flag_exitif(j));

		else if (name == "flag:once")
			ADD_JOB(JOB_flag_once(j));

		else if (name == "music:set")
			ADD_JOB(JOB_music_set(j));

		else if (name == "music:volume")
			ADD_JOB(JOB_music_volume(j));

		else if (name == "music:pause")
			ADD_JOB(job_entry(MUSIC_PAUSE));

		else if (name == "music:play")
			ADD_JOB(job_entry(MUSIC_PLAY));

		else if (name == "music:stop")
			ADD_JOB(job_entry(MUSIC_STOP));

		else if (name == "music:wait")
			ADD_JOB(JOB_music_wait(j));

		else if (name == "scene:load")
			ADD_JOB(JOB_scene_load(j));

		else if (name == "tile:replace")
			ADD_JOB(JOB_tile_replace(j));

		else if (name == "fx:fadein")
			ADD_JOB(JOB_fx_fade(FX_FADEIN));

		else if (name == "fx:fadeout")
			ADD_JOB(JOB_fx_fade(FX_FADEOUT));

		else
			std::cout << "Error: Invalid command '" << name << "'\n";
		j = j->NextSiblingElement();
	}
	return jobs_ret;
}

event_tracker::event_tracker()
{
	job_start = true;
}

bool
event_tracker::is_start()
{
	return job_start;
}

void
event_tracker::next_job()
{
	if (events.size())
	{
		++events.back().c_job;
		job_start = true;
	}
}

void
event_tracker::wait_job()
{
	job_start = false;
}

interpretor::job_entry*
event_tracker::get_job()
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

void
event_tracker::call_event(interpretor::job_list* e)
{
	events.emplace_back(e);
	job_start = true;
}

void
event_tracker::cancel_event()
{
	events.pop_back();
}

void
event_tracker::cancel_all()
{
	events.clear();
}

void
event_tracker::interrupt(interpretor::job_list* e)
{
	cancel_all();
	call_event(e);
}

void
event_tracker::queue_event(interpretor::job_list* e)
{
	events.emplace_front(e);
}
