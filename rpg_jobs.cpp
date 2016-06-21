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

interpretor::JOB_setcharacter::JOB_setcharacter(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	if (auto _name = e->Attribute("name"))
		name = _name;
	op = SETCHARACTER;
}

interpretor::JOB_movecharacter::JOB_movecharacter(tinyxml2::XMLElement* e)
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
	op = MOVECHARACTER;
}

interpretor::JOB_setglobal::JOB_setglobal(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	if (auto _name = e->Attribute("name"))
		name = _name;
	op = SETGLOBAL;
}

interpretor::JOB_ifglobal::JOB_ifglobal(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	if (auto _name = e->Attribute("name"))
		name = _name;
	remove = e->BoolAttribute("remove");
	if (auto _event = e->Attribute("event"))
		event = _event;
	else
		inline_event = parse_jobs_xml(e);
	op = IFGLOBAL;
}

interpretor::JOB_ifglobalexit::JOB_ifglobalexit(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;
	if (auto _name = e->Attribute("name"))
		name = _name;
	op = IFGLOBALEXIT;
}

interpretor::JOB_setmusic::JOB_setmusic(tinyxml2::XMLElement* e)
{
	if (auto _path = e->Attribute("path"))
		path = _path;
	loop = e->BoolAttribute("loop");
	op = SETMUSIC;
}

interpretor::JOB_newscene::JOB_newscene(tinyxml2::XMLElement* e)
{
	if (auto _path = e->Attribute("path"))
		path = _path;
	op = NEWSCENE;
}

interpretor::JOB_replacetile::JOB_replacetile(tinyxml2::XMLElement* e)
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
	op = REPLACETILE;
}

interpretor::JOB_setcyclegroup::JOB_setcyclegroup(tinyxml2::XMLElement* e)
{
	if (auto _name = e->Attribute("name"))
		group_name = _name;
	op = SETCYCLEGROUP;
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
			ADD_JOB(JOB_setcharacter(j));
		else if (name == "entity:move")
			ADD_JOB(JOB_movecharacter(j));
		else if (name == "entity:setcyclegroup")
			ADD_JOB(JOB_setcyclegroup(j));
		else if (name == "setglobal")
			ADD_JOB(JOB_setglobal(j));
		else if (name == "ifglobal")
			ADD_JOB(JOB_ifglobal(j));
		else if (name == "music:set")
			ADD_JOB(JOB_setmusic(j));
		else if (name == "music:pause")
			ADD_JOB(job_entry(PAUSEMUSIC));
		else if (name == "music:play")
			ADD_JOB(job_entry(PLAYMUSIC));
		else if (name == "music:stop")
			ADD_JOB(job_entry(STOPMUSIC));
		else if (name == "newscene")
			ADD_JOB(JOB_newscene(j));
		else if (name == "tile:replace")
			ADD_JOB(JOB_replacetile(j));
		else if (name == "ifglobalexit")
			ADD_JOB(JOB_ifglobalexit(j));

		else
			std::cout << "Error: Invalid command '" << name << "'\n";
		j = j->NextSiblingElement();
	}
	return jobs_ret;
}