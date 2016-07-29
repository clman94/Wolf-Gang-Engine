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

util::error
entity::load_entity(std::string path, texture_manager & tm)
{
	using namespace tinyxml2;
	XMLDocument doc;
	doc.LoadFile(path.c_str());

	auto ele_root = doc.RootElement();

	if (ele_root)
	{
		auto ele_animations = ele_root->FirstChildElement("animations");
		load_animations(ele_animations, tm);
	}

	return 0;
}

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
			node.set_animation(i.anim, true);
			return true;
		}
	}
	return false;
}

int
entity::draw(engine::renderer &_r)
{
	node.set_position(get_exact_position());
	node.draw(_r);
	return 0;
}

util::error
entity::load_animations(tinyxml2::XMLElement* e, texture_manager& tm)
{
	assert(e != nullptr);

	auto ele = e->FirstChildElement();
	while (ele)
	{
		animations.emplace_back();
		auto &anim = animations.back();
		anim.set_name(ele->Name());
		
		anim.type = e_type::movement;

		if (ele->Attribute("type", "user"))
			anim.type = e_type::user;

		if (ele->Attribute("type", "constant"))
			anim.type = e_type::constant;

		if (ele->Attribute("type", "movement"))
			anim.type = e_type::movement;

		if (ele->Attribute("type", "speech"))
			anim.type = e_type::speech;

		load_xml_animation(ele, anim.anim, tm);
		ele = ele->NextSiblingElement();
	}

	return 0;
}

util::error
entity::load_xml_animation(tinyxml2::XMLElement* ele, engine::animation &anim, texture_manager &tm)
{
	assert(ele != nullptr);

	int  att_frames   = ele->IntAttribute("frames");
	int  att_interval = ele->IntAttribute("interval");
	int  att_default  = ele->IntAttribute("default");
	bool att_loop     = ele->BoolAttribute("loop");
	bool att_pingpong = ele->BoolAttribute("pingpong");
	auto att_atlas    = ele->Attribute("atlas");
	auto att_tex      = ele->Attribute("tex");
	auto att_type     = ele->Attribute("type");

	if (!att_tex)   return "Please provide texture attibute for character";
	if (!att_atlas) return "Please provide atlas attribute for character";

	auto t = tm.get_texture(att_tex);
	if (!t)
		return "Texture '" + std::string(att_tex) + "' not found";

	int loop_type = anim.LOOP_NONE;
	if (att_loop)     loop_type = anim.LOOP_LINEAR;
	if (att_pingpong) loop_type = anim.LOOP_PING_PONG;
	anim.set_loop(loop_type);

	anim.add_interval(0, att_interval);
	anim.set_texture(*t);

	{
		auto atlas = t->get_entry(att_atlas);
		if (atlas.w == 0)
			return "Atlas '" + std::string(att_atlas) + "' does not exist";

		anim.generate(
			(att_frames <= 0 ? 1 : att_frames), // Default one frame
			atlas);
	}

	anim.set_default_frame(att_default);

	auto ele_seq = ele->FirstChildElement("seq");
	while (ele_seq)
	{
		anim.add_interval(
			(engine::frame_t)ele_seq->IntAttribute("from"),
			ele_seq->IntAttribute("interval"));
		ele_seq = ele_seq->NextSiblingElement();
	}

	return 0;
}

// #########
// character
// #########

character::character()
{
	cyclegroup = "default";
	move_speed = 3*TILE_SIZE.x;
}

void
character::set_cycle_group(std::string name)
{
	cyclegroup = name;
}

void
character::set_cycle(std::string name)
{
	if (cycle != name)
	{
		set_animation(cyclegroup + ":" + name);
		cycle = name;
	}
}

void
character::set_cycle(e_cycle type)
{
	switch (type)
	{
	case e_cycle::default:  set_cycle("default"); break;
	case e_cycle::left:     set_cycle("left");    break;
	case e_cycle::right:    set_cycle("right");   break;
	case e_cycle::up:       set_cycle("up");      break;
	case e_cycle::down:     set_cycle("down");    break;
	case e_cycle::idle:     set_cycle("idle");    break;
	}
}

void
character::set_speed(float f)
{
	move_speed = f;
}

float
character::get_speed()
{
	return move_speed;
}

// #########
// tilemap
// #########

tilemap::tilemap()
{
	node.set_tile_size(TILE_SIZE);
}


void
tilemap::set_texture(engine::texture& t)
{
	node.set_texture(t);
}

util::error
tilemap::load_tilemap(tinyxml2::XMLElement* e, collision_system &collision, size_t layer)
{
	assert(e != nullptr);

	if (auto path = e->Attribute("path"))
	{
		using namespace tinyxml2;
		XMLDocument doc;
		doc.LoadFile(path);
		auto root = doc.RootElement();
		return load_tilemap(root, collision, layer);
	}

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

		if (i->BoolAttribute("c"))
			collision.add_wall({ pos*TILE_SIZE, fill*TILE_SIZE });

		engine::ivector off;
		for (off.x = 0; off.x < fill.x; off.x++)
		{
			for (off.y = 0; off.y < fill.y; off.y++)
			{
				node.set_tile(pos + off, i->Name(), layer, r);
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
	node.set_position(get_exact_position());
	node.draw(_r);
	return 0;
}

// #########
// collision_system
// #########

collision_system::collision_box*
collision_system::wall_collision(const engine::frect & r)
{
	for (auto &i : walls)
	{
		if (i.is_intersect(r))
			return &i;
	}
	return nullptr;
}

collision_system::door*
collision_system::door_collision(const engine::fvector & r)
{
	for (auto &i : doors)
	{
		if (i.valid && i.is_intersect(r))
			return &i;
	}
	return nullptr;
}

collision_system::trigger*
collision_system::trigger_collision(const engine::fvector & pos)
{
	for (auto &i : triggers)
		if (i.valid && i.is_intersect(pos))
			return &i;
	return nullptr;
}

collision_system::trigger*
collision_system::button_collision(const engine::fvector & pos)
{
	for (auto &i : buttons)
		if (i.valid && i.is_intersect(pos))
			return &i;
	return nullptr;
}

void
collision_system::validate_collisionbox(collision_box & cb, flag_container & flags)
{
	if (flags.has_flag(cb.invalid_on_flag))
		cb.valid = false;
	if (!cb.spawn_flag.empty())
		flags.set_flag(cb.spawn_flag);
}

void 
collision_system::validate_all(flag_container& flags)
{
	for (auto &i : walls)    validate_collisionbox(i, flags);
	for (auto &i : doors)    validate_collisionbox(i, flags);
	for (auto &i : triggers) validate_collisionbox(i, flags);
	for (auto &i : buttons)  validate_collisionbox(i, flags);
}

void
collision_system::add_wall(engine::frect r)
{
	collision_box nw;
	nw.set_rect(r);
	walls.push_back(nw);
}

void
collision_system::clear()
{
	walls.clear();
	doors.clear();
	triggers.clear();
	buttons.clear();
}

util::error
collision_system::load_collision_boxes(tinyxml2::XMLElement* e, flag_container& flags)
{
	assert(e != nullptr);

	auto ele = e->FirstChildElement();
	while (ele)
	{
		std::string box_type = util::safe_string(ele->Name());

		std::string invalid_on_flag = util::safe_string(ele->Attribute("invalid"));
		if (flags.has_flag(invalid_on_flag))
		{
			ele = ele->NextSiblingElement();
			continue;
		}

		std::string spawn_flag = util::safe_string(ele->Attribute("spawn"));

		engine::frect rect;
		rect.x = ele->FloatAttribute("x");
		rect.y = ele->FloatAttribute("y");
		rect.w = ele->FloatAttribute("w");
		rect.h = ele->FloatAttribute("h");
		rect = engine::scale(rect, TILE_SIZE);

		if (box_type == "wall")
		{
			collision_box nw;
			nw.set_rect(rect);
			nw.invalid_on_flag = invalid_on_flag;
			nw.spawn_flag = spawn_flag;
			walls.emplace_back(nw);
		}

		if (box_type == "door")
		{
			door nd;
			nd.set_rect(rect);
			nd.invalid_on_flag = invalid_on_flag;
			nd.spawn_flag = spawn_flag;
			nd.name = util::safe_string(ele->Attribute("name"));
			nd.destination = util::safe_string(ele->Attribute("dest"));
			nd.scene_path = util::safe_string(ele->Attribute("scene"));
			doors.push_back(nd);
		}

		if (box_type == "trigger")
		{
			trigger nt;
			nt.set_rect(rect);
			nt.invalid_on_flag = invalid_on_flag;
			nt.spawn_flag = spawn_flag;
			nt.inline_event.load_xml_event(ele);
			nt.event = util::safe_string(ele->Attribute("event"));
			triggers.push_back(std::move(nt));
		}

		ele = ele->NextSiblingElement();
	}
	return 0;
}

// #########
// player_character
// #########

void
player_character::set_locked(bool l)
{
	locked = l;
}

bool
player_character::is_locked()
{
	return locked;
}

void
player_character::movement(controls& c, collision_system& collision, float delta)
{
	if (locked)
	{
		stop_withtype(entity::e_type::movement);
		return;
	}

	engine::fvector pos = get_position();
	engine::fvector move(0, 0);

	if (c.is_triggered(controls::control::left))
	{
		if (!collision.wall_collision({ pos - engine::fvector(10, 8), { 10, 8 } }))
			move.x -= get_speed();
		set_cycle(character::e_cycle::left);
	}

	if (c.is_triggered(controls::control::right))
	{
		if (!collision.wall_collision({ pos - engine::fvector(0, 8), { 10, 8 } }))
			move.x += get_speed();
		set_cycle(character::e_cycle::right);
	}

	if (c.is_triggered(controls::control::up))
	{
		if (!collision.wall_collision({ pos - engine::fvector(8, 16), { 16, 16 } }))
			move.y -= get_speed();
		set_cycle(character::e_cycle::up);
	}

	if (c.is_triggered(controls::control::down))
	{
		if (!collision.wall_collision({ pos - engine::fvector(8, 0), { 16, 4 } }))
			move.y += get_speed();
		set_cycle(character::e_cycle::down);
	}

	if (move != engine::fvector(0, 0))
	{
		set_position(pos + (move*delta));
		tick_withtype(entity::e_type::movement);
	}
	else
		stop_withtype(entity::e_type::movement);
}

engine::fvector
player_character::get_activation_point()
{
	return{ 0,0 };
}

// #########
// scene_events
// #########

void
scene_events::clear()
{
	events.clear();
}

interpreter::event*
scene_events::find_event(std::string name)
{
	for (auto &i : events)
		if (i.name == name)
			return &i.event;
	return nullptr;
}

interpreter::event_tracker&
scene_events::get_tracker()
{
	return tracker;
}

util::error
scene_events::load_event(tinyxml2::XMLElement * e)
{
	events.emplace_back();
	auto &nevent = events.back();
	nevent.event.load_xml_event(e);
	nevent.name = util::safe_string(e->Attribute("name"));
	return 0;
}

// #########
// scene
// #########

collision_system&
scene::get_collision_system()
{
	return collision;
}

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
scene::load_scene(std::string path, flag_container& flags, engine::renderer& r, texture_manager& tm)
{
	using namespace tinyxml2;

	XMLDocument doc;
	doc.LoadFile(path.c_str());

	auto root = doc.RootElement();

	// Load collision boxes
	collision.clear();
	auto ele_collisionboxes = root->FirstChildElement("collisionboxes");
	if (ele_collisionboxes)
		collision.load_collision_boxes(ele_collisionboxes, flags);

	if (auto ele_tilemap_tex = root->FirstChildElement("tilemap_texture"))
	{
		auto tex = tm.get_texture(ele_tilemap_tex->GetText());
		if (!tex)
			return "Invalid tilemap texture";
		tilemap.set_texture(*tex);
	}
	else
		return "Tilemap texture is not defined";

	// Load all tilemap layers
	tilemap.clear();
	auto ele_tilemap = root->FirstChildElement("tilemap");
	while (ele_tilemap)
	{
		int att_layer = ele_tilemap->IntAttribute("layer");
		tilemap.load_tilemap(ele_tilemap, collision, att_layer);
		ele_tilemap = ele_tilemap->NextSiblingElement("tilemap");
	}

	// Load all events
	auto ele_event = root->FirstChildElement("event");
	while (ele_event)
	{
		events.load_event(ele_event);
		ele_event = ele_event->NextSiblingElement("event");
	}
	return 0;
}

void
scene::refresh_renderer(engine::renderer & _r)
{
	_r.add_client(&tilemap);
}

// #########
// controls
// #########

controls::controls()
{
	reset();
}

void
controls::trigger(control c)
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

scene& game::get_scene()
{
	return game_scene;
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
	if (!ele_textures) return "Please specify the texture file";
	std::string textures_path = ele_textures->Attribute("path");

	auto ele_player = ele_root->FirstChildElement("player");
	if (!ele_player) return "Please specify the player";
	std::string player_path = ele_player->Attribute("path");

	textures.load_settings(textures_path);

	player.load_entity(player_path, textures);
	player.set_position({ 10, 10 });
	player.set_cycle(character::e_cycle::default);

	game_scene.load_scene(scene_path, flags, *get_renderer(), textures);
	return 0;
}

void
game::tick(controls& con)
{
	float delta = frameclock.get_elapse().s();
	player.movement(con, game_scene.get_collision_system(), delta);

	frameclock.restart();
}

void
game::refresh_renderer(engine::renderer & _r)
{
	game_scene.set_renderer(_r);
	_r.add_client(&player);
}