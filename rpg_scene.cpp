#include "rpg_scene.hpp"
#include "rpg_config.hpp"
#include "rpg_jobs.hpp"

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

util::error
scene::parse_collisionbox_xml(tinyxml2::XMLElement* e)
{
	using namespace tinyxml2;

	collisionboxes.clear();

	XMLElement* c = e->FirstChildElement();
	while (c)
	{
		std::string name = c->Name();

		collisionboxes.emplace_back();
		collisionbox& nbox = collisionboxes.back();
		nbox.pos.x = c->FloatAttribute("x");
		nbox.pos.y = c->FloatAttribute("y");
		nbox.pos *= TILE_SIZE;
		nbox.size.x = c->FloatAttribute("w");
		nbox.size.y = c->FloatAttribute("h");
		nbox.size *= TILE_SIZE;

		if (auto _name = c->Attribute("name"))
			nbox.name = _name;

		if (auto _event = c->Attribute("event"))
			nbox.event = _event;
		else
			nbox.inline_event = interpretor::parse_jobs_xml(c);

		if (auto _bind = c->Attribute("bind"))
			nbox.bind_flag = _bind;
		if (auto _if = c->Attribute("if"))
			nbox.if_flag = _if;

		if (name == "box")
			nbox.type = collisionbox::box_type::TOUCH_EVENT;

		else if (name == "button")
			nbox.type = collisionbox::box_type::BUTTON;

		else if (name == "wall")
			nbox.type   = collisionbox::box_type::WALL;

		else if (name == "door")
		{
			const char* att_dest = nullptr;
			const char* att_path = nullptr;

			att_dest = c->Attribute("dest");
			att_path = c->Attribute("path");

			if(!att_dest)
				return "Please provide destination for door";
			if (!att_path)
				return "Please provide the path of new scene";

			{
				using namespace interpretor;
				auto j1 = new JOB_fx_fade(FX_FADEOUT);
				auto j2 = new JOB_scene_load();
				j2->door = att_dest;
				j2->path = att_path;
				nbox.inline_event.push_back(engine::ptr_GC<job_entry>::take(j1));
				nbox.inline_event.push_back(engine::ptr_GC<job_entry>::take(j2));
			}

			nbox.type = collisionbox::box_type::DOOR;
		}

		else
			return "Invalid collisionbox type '" + name + "'\n";
		c = c->NextSiblingElement();
	}
	return 0;
}