#include "rpg_scene.hpp"
#include "rpg_config.hpp"

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

utility::error
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
			collisionboxes.emplace_back();
			collisionbox& nbox = collisionboxes.back();
			nbox.pos.x  = c->FloatAttribute("x");
			nbox.pos.y  = c->FloatAttribute("y");
			nbox.pos   *= TILE_SIZE;
			nbox.size.x = c->FloatAttribute("w");
			nbox.size.y = c->FloatAttribute("h");
			nbox.size  *= TILE_SIZE;
			if (auto eventname = c->Attribute("event"))
				nbox.name = eventname;
			else
				nbox.inline_event = interpretor::parse_jobs_xml(c);
			if (auto f = c->Attribute("bind"))
				nbox.bind_flag = f;
			if (auto f = c->Attribute("if"))
				nbox.if_flag = f;
			nbox.type = collisionbox::box_type::TOUCH_EVENT;
		}
		else if (name == "button")
		{
			collisionboxes.emplace_back();
			collisionbox& nbox = collisionboxes.back();
			nbox.pos.x  = c->FloatAttribute("x");
			nbox.pos.y  = c->FloatAttribute("y");
			nbox.pos   *= TILE_SIZE;
			nbox.size.x = c->FloatAttribute("w");
			nbox.size.y = c->FloatAttribute("h");
			nbox.size  *= TILE_SIZE;
			if (auto eventname = c->Attribute("event"))
				nbox.name = eventname;
			else
				nbox.inline_event = interpretor::parse_jobs_xml(c);
			if (auto f = c->Attribute("bind"))
				nbox.bind_flag = f;
			if (auto f = c->Attribute("if"))
				nbox.if_flag = f;
			nbox.type = collisionbox::box_type::BUTTON;
		}
		else if (name == "wall")
		{
			collisionboxes.emplace_back();
			collisionbox& nbox = collisionboxes.back();
			nbox.pos.x  = c->FloatAttribute("x");
			nbox.pos.y  = c->FloatAttribute("y");
			nbox.pos   *= TILE_SIZE;
			nbox.size.x = c->FloatAttribute("w");
			nbox.size.y = c->FloatAttribute("h");
			nbox.size  *= TILE_SIZE;
			nbox.type   = collisionbox::box_type::WALL;
		}
		else
			return "Invalid collisionbox type '" + name + "'\n";
		c = c->NextSiblingElement();
	}
	return 0;
}