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

void
entity::set_dynamic_depth(bool a)
{
	dynamic_depth = a;
}

entity::entity()
{
	dynamic_depth = true;
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
	if (dynamic_depth)
	{
		float ndepth = util::clamp(get_position().y / 32, 
			defs::TILE_DEPTH_RANGE_MIN, defs::TILE_DEPTH_RANGE_MAX);
		if (ndepth != get_depth())
			set_depth(ndepth);
	}
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
	move_speed = 3* defs::TILE_SIZE.x;
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
	node.set_tile_size(defs::TILE_SIZE);
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
			collision.add_wall({ pos*defs::TILE_SIZE, fill*defs::TILE_SIZE });

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

util::error
tilemap::load_scene_tilemap(tinyxml2::XMLElement * e, collision_system & collision)
{
	auto ele_tilemap = e->FirstChildElement("tilemap");
	while (ele_tilemap)
	{
		int att_layer = ele_tilemap->IntAttribute("layer");
		load_tilemap(ele_tilemap, collision, att_layer);
		ele_tilemap = ele_tilemap->NextSiblingElement("tilemap");
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
		bool is_valid = !flags.has_flag(invalid_on_flag);

		std::string spawn_flag = util::safe_string(ele->Attribute("spawn"));

		engine::frect rect;
		rect.x = ele->FloatAttribute("x");
		rect.y = ele->FloatAttribute("y");
		rect.w = ele->FloatAttribute("w");
		rect.h = ele->FloatAttribute("h");
		rect = engine::scale(rect, defs::TILE_SIZE);
		

		// ##### TODO: Refactor #####

		if (box_type == "wall")
		{
			collision_box nw;
			nw.set_rect(rect);
			nw.invalid_on_flag = invalid_on_flag;
			nw.spawn_flag = spawn_flag;
			nw.valid = is_valid;
			walls.emplace_back(nw);
		}

		if (box_type == "door")
		{
			door nd;
			nd.set_rect(rect);
			nd.invalid_on_flag = invalid_on_flag;
			nd.spawn_flag = spawn_flag;
			nd.valid = is_valid;
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
			nt.valid = is_valid;
			nt.inline_event.load_xml_event(ele);
			nt.event = util::safe_string(ele->Attribute("event"));
			triggers.push_back(std::move(nt));
		}

		if (box_type == "button")
		{
			trigger nt;
			nt.set_rect(rect);
			nt.invalid_on_flag = invalid_on_flag;
			nt.spawn_flag = spawn_flag;
			nt.valid = is_valid;
			nt.inline_event.load_xml_event(ele);
			nt.event = util::safe_string(ele->Attribute("event"));
			buttons.push_back(std::move(nt));
		}

		ele = ele->NextSiblingElement();
	}
	return 0;
}

// #########
// player_character
// #########

player_character::player_character()
{
	facing_direction = direction::other;
}

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
		facing_direction = direction::left;
		set_cycle(character::e_cycle::left);
	}

	if (c.is_triggered(controls::control::right))
	{
		if (!collision.wall_collision({ pos - engine::fvector(0, 8), { 10, 8 } }))
			move.x += get_speed();
		facing_direction = direction::right;
		set_cycle(character::e_cycle::right);
	}

	if (c.is_triggered(controls::control::up))
	{
		if (!collision.wall_collision({ pos - engine::fvector(8, 16), { 16, 16 } }))
			move.y -= get_speed();
		facing_direction = direction::up;
		set_cycle(character::e_cycle::up);
	}

	if (c.is_triggered(controls::control::down))
	{
		if (!collision.wall_collision({ pos - engine::fvector(8, 0), { 16, 4 } }))
			move.y += get_speed();
		facing_direction = direction::down;
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
player_character::get_activation_point(float distance)
{
	switch (facing_direction)
	{
	case direction::other: return get_position();
	case direction::left:  return get_position() + engine::fvector(-distance, 0);
	case direction::right: return get_position() + engine::fvector(distance, 0);
	case direction::up:    return get_position() + engine::fvector(0, -distance);
	case direction::down:  return get_position() + engine::fvector(0, distance);
	}
	return{ 0, 0 };
}

// #########
// scene_events
// #########

void
scene_events::clear()
{
	tracker.cancel_all();
	events.clear();
}

int
scene_events::trigger_event(std::string name)
{
	auto e = find_event(name);
	if (!e) return 1;
	tracker.call_event(e);
	return 0;
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

util::error
scene_events::load_scene_events(tinyxml2::XMLElement * e)
{
	auto ele_event = e->FirstChildElement("event");
	while (ele_event)
	{
		load_event(ele_event);
		ele_event = ele_event->NextSiblingElement("event");
	}
	return 0;
}

// #########
// scene
// #########

void
scene::activate_trigger(collision_system::trigger& trigger, flag_container& flags)
{
	if (trigger.inline_event.get_op_count())
		events.get_tracker().call_event(&trigger.inline_event);
	events.trigger_event(trigger.event);
	collision.validate_collisionbox(trigger, flags);
}

bool
scene::player_button_activate(engine::fvector pos, flag_container& flags)
{
	auto hit = collision.button_collision(pos);
	if (!hit) return false;
	activate_trigger(*hit, flags);
	return true;
}

bool
scene::player_trigger_activate(engine::fvector pos, flag_container& flags)
{
	auto hit = collision.trigger_collision(pos);
	if (!hit) return false;
	activate_trigger(*hit, flags);
	return false;
}

scene::scene()
{
	tilemap.set_depth(defs::TILES_DEPTH);
	add_child(tilemap);
	add_child(characters);
	add_child(entities);
}

scene_events&
scene::get_events()
{
	return events;
}

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



void
scene::clean_scene()
{
	tilemap.clear();
	collision.clear();
	events.clear();
	characters.clear();
	entities.clear();
}

util::error
scene::load_entities(tinyxml2::XMLElement * e, texture_manager& tm)
{
	auto ele = e->FirstChildElement();
	while (ele)
	{
		std::string name = ele->Name();
		std::string att_path = ele->Attribute("path");
		float x = ele->FloatAttribute("x");
		float y = ele->FloatAttribute("y");

		auto& ne = entities.add_item();
		ne.load_entity(att_path, tm);
		ne.set_position({ x, y });
		get_renderer()->add_client(&ne);

		ele = ele->NextSiblingElement();
	}
	return 0;
}

util::error
scene::load_characters(tinyxml2::XMLElement * e, texture_manager& tm)
{
	auto ele = e->FirstChildElement();
	while (ele)
	{
		std::string name = ele->Name();
		std::string att_path = ele->Attribute("path");
		float x = ele->FloatAttribute("x");
		float y = ele->FloatAttribute("y");

		auto& ne = characters.add_item();
		ne.load_entity(att_path, tm);
		ne.set_position({ x, y });
		get_renderer()->add_client(&ne);

		ele = ele->NextSiblingElement();
	}
	return 0;
}

util::error
scene::load_scene(std::string path, flag_container& flags, texture_manager& tm)
{
	using namespace tinyxml2;

	XMLDocument doc;
	doc.LoadFile(path.c_str());

	auto root = doc.RootElement();

	clean_scene();

	// Load collision boxes
	auto ele_collisionboxes = root->FirstChildElement("collisionboxes");
	if (ele_collisionboxes)
		collision.load_collision_boxes(ele_collisionboxes, flags);

	// Load tilemap texture
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
	tilemap.load_scene_tilemap(root, collision);

	// Load all events
	events.load_scene_events(root);
	events.trigger_event("_start_");

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

// #########
// game
// #########

game::game()
{
	root_node.add_child(player);
	root_node.add_child(game_scene);
}

void
game::player_scene_interact(controls& con)
{
	if (con.is_triggered(controls::control::activate))
		game_scene.player_button_activate(player.get_activation_point(), flags);
	game_scene.player_trigger_activate(player.get_position(), flags);
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

	game_scene.load_scene(scene_path, flags, textures);

	if (auto ele_narrative = ele_root->FirstChildElement("narrative"))
	{
		narrative.load_narrative(ele_narrative, textures);
	}
	return 0;
}

int
game::tick_interpretor(controls& con, scene_events& events)
{
	auto& tracker = events.get_tracker();

	do {
		auto op = tracker.get_current_op();
		if (!op)
		{
			narrative.hide_box();
			player.set_locked(false);
			return 0;
		}

		using namespace interpreter;
		switch (op->get_opcode())
		{
		case e_opcode::say:
		{
			auto _op = op->cast_to<OP_say>();
			if (tracker.is_start())
			{
				player.set_locked(true);
				narrative.show_box();
				narrative.set_interval(_op->interval);
				narrative.reveal_text(_op->text, _op->append);
			}
			tracker.wait_until(!narrative.is_revealing());
			break;
		}
		case e_opcode::waitforkey:
		{
			if (tracker.is_start())
			{
				player.set_locked(true);
			}
			bool is_triggered = con.is_triggered(controls::control::activate);
			tracker.wait_until(is_triggered);
			if (is_triggered)
				player.set_locked(false);
			break;
		}
		case e_opcode::wait:
		{
			auto _op = op->cast_to<OP_wait>();
			if (tracker.is_start())
			{
				_op->clock.restart();
			}
			bool is_reached = _op->clock.get_elapse().s() >= _op->seconds;
			tracker.wait_until(is_reached);
			break;
		}
		case e_opcode::flag_set:
		{
			auto _op = op->cast_to<OP_flag_set>();
			flags.set_flag(_op->flag);
			tracker.next();
			break;
		}
		case e_opcode::flag_unset:
		{
			auto _op = op->cast_to<OP_flag_unset>();
			flags.unset_flag(_op->flag);
			tracker.next();
			break;
		}
		}

	} while (tracker.is_start());

	return 0;
}

void
game::tick(controls& con)
{
	float delta = frameclock.get_elapse().s();

	player.movement(con, game_scene.get_collision_system(), delta);

	if (!player.is_locked())
	{
		root_node.set_focus(player.get_position());
		player_scene_interact(con);
	}

	tick_interpretor(con, game_scene.get_events());

	frameclock.restart();
}

void
game::refresh_renderer(engine::renderer & r)
{
	game_scene.set_renderer(r);
	r.add_client(&player);
	root_node.set_viewport(r.get_size());
	root_node.set_boundary({ 11*32, 11*32 });

	r.add_client(&narrative);
}

// ##########
// panning_node
// ##########

void
panning_node::set_boundary(engine::fvector a)
{
	boundary = a;
}

void
panning_node::set_viewport(engine::fvector a)
{
	viewport = a;
}

void
panning_node::set_focus(engine::fvector pos)
{
	engine::fvector npos = pos - (viewport * 0.5f);
	npos.x = util::clamp(npos.x, 0.f, boundary.x - viewport.x);
	npos.y = util::clamp(npos.y, 0.f, boundary.y - viewport.y);
	set_position(-npos);
}

// ##########
// narrative
// ##########

util::error
narrative_dialog::load_box(tinyxml2::XMLElement* e, texture_manager& tm)
{
	auto ele_box = e->FirstChildElement("box");

	std::string att_box_tex = ele_box->Attribute("tex");
	std::string att_box_atlas = ele_box->Attribute("atlas");

	auto tex = tm.get_texture(att_box_tex);
	if (!tex) return "Texture does not exist";
	box.set_texture(*tex, att_box_atlas);

	set_box_position(position::bottom);

	return 0;
}

util::error
narrative_dialog::load_font(tinyxml2::XMLElement* e)
{
	auto ele_font = e->FirstChildElement("font");

	std::string att_font_path = ele_font->Attribute("path");

	font.load(att_font_path);
	text.set_font(font);
	text.set_scale(0.5f);
	return 0;
}

narrative_dialog::narrative_dialog()
{
	revealing = false;
	hide_box();

	interval = 100;

	box.add_child(text);
}

void
narrative_dialog::set_box_position(position pos)
{
	float offx = (defs::DISPLAY_SIZE.x - box.get_size().x) / 2;

	switch (pos)
	{
	case position::top:
	{
		box.set_position({ offx, 10 });
		break;
	}
	case position::bottom:
	{
		box.set_position({ offx, defs::DISPLAY_SIZE.y - box.get_size().y - 10 });
		break;
	}
	}
}

bool
narrative_dialog::is_revealing()
{
	return revealing;
}

void
narrative_dialog::reveal_text(std::string str, bool append)
{
	timer.restart();
	revealing = true;

	if (append)
		full_text += str;
	else
	{
		full_text = str;
		text.set_text("");
		c_char = 0;
	}
}

void
narrative_dialog::instant_text(std::string str, bool append)
{
	revealing = false;
	if (append)
		full_text += str;
	else
		full_text = str;
	text.set_text(full_text);
}

void
narrative_dialog::show_box()
{
	text.set_visible(true);
	box.set_visible(true);
}

void
narrative_dialog::hide_box()
{
	revealing = false;
	text.set_visible(false);
	text.set_text("");
	box.set_visible(false);
}

bool rpg::narrative_dialog::is_box_open()
{
	return box.is_visible();
}

void
narrative_dialog::set_interval(float ms)
{
	interval = ms;
}

util::error
narrative_dialog::load_narrative(tinyxml2::XMLElement* e, texture_manager& tm)
{
	load_box(e, tm);
	load_font(e);
	return 0;
}

int
narrative_dialog::draw(engine::renderer& r)
{
	if (!revealing) return 0;

	float time = timer.get_elapse().ms();
	if (time >= interval)
	{
		c_char += (size_t)time / (size_t)interval;
		c_char  = util::clamp<size_t>(c_char, 0, full_text.size());

		std::string display(full_text.begin(), full_text.begin() + c_char);
		text.set_text(display);

		if (c_char >= full_text.size())
			revealing = false;

		timer.restart();
	}
	return 0;
}

void
narrative_dialog::refresh_renderer(engine::renderer& r)
{
	r.add_client(&box);
	r.add_client(&text);
}
