#include "rpg_scene.hpp"

using namespace rpg;

int
scene::parse_events_xml(tinyxml2::XMLElement* e)
{
	// load onactivate events
	tinyxml2::XMLElement* onactivate_e = e->FirstChildElement("event");
	while (onactivate_e)
	{
		events.push_back({ onactivate_e->Attribute("name"), interpretor::parse_jobs_xml(onactivate_e) });
		onactivate_e = onactivate_e->NextSiblingElement("event");
	}
	return 0;
}

int
scene::parse_collisionbox_xml(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;

	collisionboxes.clear();

	XMLElement* c = e->FirstChildElement();
	while (c)
	{
		std::string name = c->Name();
		if (name == "box")
		{
			collisionbox nbox;
			nbox.pos.x = c->FloatAttribute("x") * 32;
			nbox.pos.y = c->FloatAttribute("y") * 32;
			nbox.size.x = c->FloatAttribute("w") * 32;
			nbox.size.y = c->FloatAttribute("h") * 32;
			if (auto eventname = c->Attribute("event"))
				nbox.name = eventname;
			else
				nbox.inline_event = interpretor::parse_jobs_xml(c);
			nbox.once = c->BoolAttribute("once");
			nbox.type = nbox.TOUCH_EVENT;
			nbox.triggered = false;
			collisionboxes.push_back(nbox);
		}
		else if (name == "button")
		{
			collisionbox nbox;
			nbox.pos.x = c->FloatAttribute("x") * 32;
			nbox.pos.y = c->FloatAttribute("y") * 32;
			nbox.size.x = c->FloatAttribute("w") * 32;
			nbox.size.y = c->FloatAttribute("h") * 32;
			if (auto eventname = c->Attribute("event"))
				nbox.name = eventname;
			else
				nbox.inline_event = interpretor::parse_jobs_xml(c);
			nbox.once = c->BoolAttribute("once");
			nbox.type = nbox.BUTTON;
			nbox.triggered = false;
			collisionboxes.push_back(nbox);
		}
		else if (name == "wall")
		{
			collisionbox nbox;
			nbox.pos.x = c->FloatAttribute("x") * 32;
			nbox.pos.y = c->FloatAttribute("y") * 32;
			nbox.size.x = c->FloatAttribute("w") * 32;
			nbox.size.y = c->FloatAttribute("h") * 32;
			nbox.type = nbox.WALL;
			collisionboxes.push_back(nbox);
		}
		else
		{
			std::cout << "Error: Invalid collisionbox type '" << name << "'\n";
		}
		c = c->NextSiblingElement();
	}
	return 0;
}