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
	op = MUSIC_SET;
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

interpretor::JOB_entity_spawn::JOB_entity_spawn(tinyxml2::XMLElement* e)
{
	if (auto _as = e->Attribute("as"))
		as = _as;
	if (auto _path = e->Attribute("path"))
		path = _path;
	op = ENTITY_SPAWN;
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

#define ADD_JOB(A) jobs_ret.push_back(new A)
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
		else if (name == "entity:setdirection")
			ADD_JOB(JOB_entity_setdirection(j));
		else if (name == "flag:set")
			ADD_JOB(JOB_flag_set(j));
		else if (name == "flag:if")
			ADD_JOB(JOB_flag_if(j));
		else if (name == "flag:exitif")
			ADD_JOB(JOB_flag_exitif(j));
		else if (name == "flag:once")
			ADD_JOB(JOB_flag_once(j));
		else if (name == "music:set")
			ADD_JOB(JOB_music_set(j));
		else if (name == "music:pause")
			ADD_JOB(job_entry(MUSIC_PAUSE));
		else if (name == "music:play")
			ADD_JOB(job_entry(MUSIC_PLAY));
		else if (name == "music:stop")
			ADD_JOB(job_entry(MUSIC_STOP));
		else if (name == "scene:load")
			ADD_JOB(JOB_scene_load(j));
		else if (name == "tile:replace")
			ADD_JOB(JOB_tile_replace(j));

		else
			std::cout << "Error: Invalid command '" << name << "'\n";
		j = j->NextSiblingElement();
	}
	return jobs_ret;
}