#include "rpg.hpp"

using namespace rpg;

// #########
// flag_container
// #########

bool flag_container::set_flag(std::string name)
{
	return flags.emplace(name).second;
}
bool flag_container::unset_flag(std::string name)
{
	return flags.erase(name) == 1;
}
bool flag_container::has_flag(std::string name)
{
	return flags.find(name) != flags.end();
}

// #########
// entity
// #########

entity::entity()
{
	node.set_anchor(engine::anchor::bottom);
}

void
entity::play_withtype(e_type type)
{
	if (c_animation
		&& c_animation->type == type)
		node.start();
}

void
entity::stop_withtype(e_type type)
{
	if (c_animation
		&& c_animation->type == type)
		node.stop();
}

void
entity::tick_withtype(e_type type)
{
	if (c_animation
		&& c_animation->type == type)
		node.tick();
}

bool
entity::set_animation(std::string name)
{
	for (auto &i : animations)
	{
		if (i.get_name() == name)
		{
			c_animation = &i;
			node.set_animation(i.anim);
			return true;
		}
	}
	return false;
}

int
entity::draw(engine::renderer &_r)
{
	node.set_position(get_position());
	node.draw(_r);
	return 0;
}

util::error
entity::load_animations(tinyxml2::XMLElement* e, texture_manager& tm)
{
	assert(e != nullptr);

	return 0;
}

// #########
// character
// #########

character::character()
{
	cyclegroup = "default";
	cycle = "default";
}

void
character::set_cycle_group(std::string name)
{
	cyclegroup = name;
}

void
character::set_cycle(std::string name)
{
	if (cycle != name
		&& set_animation(cyclegroup + ":" + name))
		cycle = name;
}

void
character::set_cycle(e_cycle type)
{
	switch (type)
	{
	case e_cycle::left:  set_cycle("left");  break;
	case e_cycle::right: set_cycle("right"); break;
	case e_cycle::up:    set_cycle("up");    break;
	case e_cycle::down:  set_cycle("down");  break;
	case e_cycle::idle:  set_cycle("idle");  break;
	}
}

// #########
// tilemap
// #########

tilemap::tilemap()
{
	node.set_tile_size(rpg::TILE_SIZE);
}

void
tilemap::set_texture(engine::texture& t)
{
	node.set_texture(t);
}

util::error
tilemap::load_tilemap(tinyxml2::XMLElement* e, size_t layer)
{
	assert(e != nullptr);

	auto i = e->FirstChildElement();
	while (i)
	{
		engine::ivector pos;
		pos.x = i->IntAttribute("x");
		pos.y = i->IntAttribute("y");

		engine::ivector fill;
		fill.x = i->IntAttribute("w");
		fill.y = i->IntAttribute("h");

		fill.x = fill.x <= 0 ? 1 : fill.x; // Default 1
		fill.y = fill.y <= 0 ? 1 : fill.y;

		int r = i->IntAttribute("r") % 4;

		engine::ivector npos;
		for (npos.x = 0; npos.x < fill.x; npos.x++)
		{
			for (npos.y = 0; npos.y < fill.y; npos.y++)
			{
				node.set_tile(pos + npos, i->Name(), layer, r);
			}
		}

		i = i->NextSiblingElement();
	}
	return 0;
}

void
tilemap::clear()
{
	node.clear_all();
}

int
tilemap::draw(engine::renderer &_r)
{
	node.set_position(get_position());
	node.draw(_r);
	return 0;
}

// #########
// collision_system
// #########

util::error
collision_system::load_collision_boxes(tinyxml2::XMLElement* e, flag_container& flags)
{
	assert(e != nullptr);

	walls.clear();
	doors.clear();
	triggers.clear();
	buttons.clear();

	auto ele = e->FirstChildElement();
	while (ele)
	{
		std::string box_type = util::safe_string(ele->Name());

		std::string invalid_on_flag = util::safe_string(ele->Attribute("nflag"));
		if (flags.has_flag(invalid_on_flag))
		{
			ele = ele->NextSiblingElement();
			continue;
		}

		engine::frect rect;
		rect.x = ele->IntAttribute("x");
		rect.y = ele->IntAttribute("y");
		rect.w = ele->IntAttribute("w");
		rect.h = ele->IntAttribute("h");

		if (box_type == "wall")
		{
			collision_box nw;
			nw.set_rect(rect);
			nw.invalid_on_flag = invalid_on_flag;
			walls.emplace_back(nw);
		}

		if (box_type == "door")
		{
			door nd;
			nd.set_rect(rect);
			nd.invalid_on_flag = invalid_on_flag;
			nd.name = util::safe_string(ele->Attribute("name"));
			nd.destination = util::safe_string(ele->Attribute("dest"));
			nd.scene_path = util::safe_string(ele->Attribute("scene"));
			doors.push_back(nd);
		}

		if (box_type == "trigger")
		{
			trigger nt;

		}

		ele = ele->NextSiblingElement();
	}
	return 0;
}

// #########
// player_character
// #########

engine::fvector
player_character::get_activation_point()
{
	return{ 0,0 };
}

// #########
// scene
// #########

inline character*
scene::find_character(std::string name)
{
	for (auto &i : characters)
		if (i.get_name() == name)
			return &i;
	return nullptr;
}

inline entity*
scene::find_entity(std::string name)
{
	for (auto &i : entities)
		if (i.get_name() == name)
			return &i;
	return nullptr;
}

util::error
scene::load_scene(std::string path, engine::renderer& r, texture_manager& tm)
{
	using namespace tinyxml2;

	XMLDocument doc;
	doc.LoadFile(path.c_str());

	auto root = doc.RootElement();

	if (auto ele_tilemap_tex = root->FirstChildElement("tilemap_texture"))
	{
		auto tex = tm.get_texture(ele_tilemap_tex->GetText());
		if (!tex)
			return "Invalid tilemap texture";
		tilemap.set_texture(*tex);
	}
	else
		return "Tilemape texture is not defined";

	// Load all tilemap layers
	tilemap.clear();
	auto ele_tilemap = root->FirstChildElement("tilemap");
	while (ele_tilemap)
	{
		int att_layer = ele_tilemap->IntAttribute("layer");
		tilemap.load_tilemap(ele_tilemap, att_layer);
		ele_tilemap = ele_tilemap->NextSiblingElement("tilemap");
	}

	// Load all events
	auto ele_event = root->FirstChildElement("event");
	while (ele_event)
	{
		//std::string name = ele_event->Attribute("name");

		ele_event = ele_event->NextSiblingElement("event");
	}
	return 0;
}

inline void scene::refresh_renderer(engine::renderer & _r)
{
	_r.add_client(&tilemap);
}

// #########
// controls
// #########

void
controls::trigger_control(control c)
{
	c_controls[(int)c] = true;
}

bool
controls::is_triggered(control c)
{
	return c_controls[(int)c];
}

void
controls::reset()
{
	c_controls.assign(false);
}

util::error
game::load_game(std::string path)
{
	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(path.c_str()))
		return "Could not load game file at '" + path + "'";

	auto ele_root = doc.RootElement();

	auto ele_scene = ele_root->FirstChildElement("scene");
	if (!ele_scene) return "Please specify the scene to start with";
	std::string scene_path = ele_scene->Attribute("path");

	auto ele_textures = ele_root->FirstChildElement("textures");
	if (!ele_textures) return "Please specify the scene to start with";
	std::string textures_path = ele_textures->Attribute("path");

	textures.load_settings(textures_path);

	scene.load_scene(scene_path, *get_renderer(), textures);
	return 0;
}

void
game::tick()
{

}

inline void game::refresh_renderer(engine::renderer & _r)
{
	scene.set_renderer(_r);
}
