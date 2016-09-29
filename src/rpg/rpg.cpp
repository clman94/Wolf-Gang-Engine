#include <rpg/rpg.hpp>

#include <angelscript/add_on/scriptstdstring/scriptstdstring.h>
#include <angelscript/add_on/scriptmath/scriptmath.h>
#include <angelscript/add_on/scriptarray/scriptarray.h>

#include <engine/parsers.hpp>

#include "../tinyxml2/xmlshortcuts.hpp"

#include <functional>
#include <algorithm>

using namespace rpg;
using namespace AS;

// #########
// flag_container
// #########

bool flag_container::set_flag(const std::string& pName)
{
	return flags.emplace(pName).second;
}
bool flag_container::unset_flag(const std::string& pName)
{
	return flags.erase(pName) == 1;
}
bool flag_container::has_flag(const std::string& pName)
{
	return flags.find(pName) != flags.end();
}

void flag_container::load_script_interface(script_system & pScript)
{
	pScript.add_function("bool has_flag(const string &in)", asMETHOD(flag_container, has_flag), this);
	pScript.add_function("bool set_flag(const string &in)", asMETHOD(flag_container, set_flag), this);
	pScript.add_function("bool unset_flag(const string &in)", asMETHOD(flag_container, unset_flag), this);
}

// #########
// entity
// #########

util::error
entity::load_entity(std::string pName, texture_manager & pTexture_manager)
{
	mTexture = pTexture_manager.get_texture(pName);
	if (!mTexture)
		util::error("Cannot find texture for entity");
	return 0;
}

void
entity::set_dynamic_depth(bool a)
{
	dynamic_depth = a;
}

void entity::set_anchor(engine::anchor pAnchor)
{
	mSprite.set_anchor(pAnchor);
}

void entity::set_color(engine::color pColor)
{
	mSprite.set_color(pColor);
}

void entity::set_rotation(float pRotation)
{
	mSprite.set_rotation(pRotation);
}

bool entity::is_playing()
{
	return mSprite.is_playing();
}

entity::entity()
{
	dynamic_depth = true;
	mSprite.set_anchor(engine::anchor::bottom);
	add_child(mSprite);
}

void
entity::play_animation()
{
	mSprite.start();
}

void
entity::stop_animation()
{
	mSprite.stop();
}

void
entity::tick_animation()
{
	mSprite.tick();
}

bool
entity::set_animation(const std::string& pName, bool pSwap)
{
	if (!mTexture)
		return false;

	auto animation = mTexture->get_animation(pName);
	if (!animation)
		return false;

	mSprite.set_animation(*animation, pSwap);
	return true;
}

int
entity::draw(engine::renderer &_r)
{
	if (dynamic_depth)
	{
		float ndepth = defs::TILE_DEPTH_RANGE_MAX
			- util::clamp(get_position().y / 32
				, defs::TILE_DEPTH_RANGE_MIN
				, defs::TILE_DEPTH_RANGE_MAX);
		if (ndepth != get_depth())
			set_depth(ndepth);
	}
	mSprite.draw(_r);
	return 0;
}

// #########
// character
// #########

character::character()
{
	cyclegroup = "default";
	move_speed = 3.f* defs::TILE_SIZE.x;
}

void
character::set_cycle_group(std::string name)
{
	cyclegroup = name;
	set_cycle(cycle);
}

void
character::set_cycle(const std::string& name)
{
	if (cycle != name)
	{
		set_animation(cyclegroup + ":" + name, is_playing());
		cycle = name;
	}
}

void
character::set_cycle(e_cycle type)
{
	switch (type)
	{
	case e_cycle::def:   set_cycle("default"); break;
	case e_cycle::left:  set_cycle("left");    break;
	case e_cycle::right: set_cycle("right");   break;
	case e_cycle::up:    set_cycle("up");      break;
	case e_cycle::down:  set_cycle("down");    break;
	case e_cycle::idle:  set_cycle("idle");    break;
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
// collision_system
// #########

collision_box*
collision_system::wall_collision(const engine::frect& r)
{
	for (auto &i : mWalls)
		if (i.is_valid() && i.is_intersect(r))
			return &i;
	return nullptr;
}

door*
collision_system::door_collision(const engine::fvector& pPosition)
{
	for (auto &i : mDoors)
		if (i.is_valid() && i.is_intersect(pPosition))
			return &i;
	return nullptr;
}

trigger*
collision_system::trigger_collision(const engine::fvector& pPosition)
{
	for (auto &i : mTriggers)
		if (i.is_valid() && i.is_intersect(pPosition))
			return &i;
	return nullptr;
}

trigger*
collision_system::button_collision(const engine::fvector& pPosition)
{
	for (auto &i : mButtons)
		if (i.is_valid() && i.is_intersect(pPosition))
			return &i;
	return nullptr;
}

engine::fvector
collision_system::get_door_entry(std::string pName)
{
	for (auto& i : mDoors)
	{
		if (i.name == pName)
		{
			return i.get_offset() + (i.get_size()*0.5f) + i.offset;
		}
	}
	return{ -1, -1};
}

void 
collision_system::validate_all(flag_container& pFlags)
{
	for (auto &i : mWalls)    i.validate(pFlags);
	for (auto &i : mDoors)    i.validate(pFlags);
	for (auto &i : mTriggers) i.validate(pFlags);
	for (auto &i : mButtons)  i.validate(pFlags);
}

void
collision_system::add_wall(engine::frect r)
{
	collision_box nw;
	nw.set_rect(r);
	mWalls.push_back(nw);
}

void
collision_system::add_trigger(trigger & t)
{
	mTriggers.push_back(t);
}

void
collision_system::add_button(trigger & t)
{
	mButtons.push_back(t);
}


void
collision_system::clean()
{
	mWalls.clear();
	mDoors.clear();
	mTriggers.clear();
	mButtons.clear();
}

util::error
collision_system::load_collision_boxes(tinyxml2::XMLElement* pEle)
{
	assert(pEle != nullptr);

	auto ele_item = pEle->FirstChildElement();
	while (ele_item)
	{
		std::string box_type = util::safe_string(ele_item->Name());

		if (box_type == "wall")
		{
			collision_box nw;
			nw.load_xml(ele_item);
			mWalls.emplace_back(nw);
		}

		if (box_type == "door")
		{
			door nd;
			nd.load_xml(ele_item);
			nd.name = util::safe_string(ele_item->Attribute("name"));
			nd.destination = util::safe_string(ele_item->Attribute("destination"));
			nd.scene_path = util::safe_string(ele_item->Attribute("scene"));
			nd.offset.x = ele_item->FloatAttribute("offsetx")*32;
			nd.offset.y = ele_item->FloatAttribute("offsety")*32;
			mDoors.push_back(nd);
		}

		ele_item = ele_item->NextSiblingElement();
	}
	return 0;
}

// #########
// player_character
// #########

void player_character::set_move_direction(engine::fvector pVec)
{
	if (std::abs(pVec.x) > std::abs(pVec.y))
	{
		if (pVec.x > 0)
		{
			mFacing_direction = direction::right;
			set_cycle(character::e_cycle::right);
		}
		else {
			mFacing_direction = direction::left;
			set_cycle(character::e_cycle::left);
		}
	}
	else {
		if (pVec.y > 0)
		{
			mFacing_direction = direction::down;
			set_cycle(character::e_cycle::down);
		}
		else {
			mFacing_direction = direction::up;
			set_cycle(character::e_cycle::up);
		}
	}
}

player_character::player_character()
{
	mFacing_direction = direction::other;
}

void
player_character::set_locked(bool pLocked)
{
	mLocked = pLocked;
}

bool
player_character::is_locked()
{
	return mLocked;
}

engine::frect player_character::get_collision(direction pDirection)
{
	const engine::fvector pos = get_position();
	const engine::fvector size = get_size() - engine::fvector(2, 2);
	const engine::fvector hsize = get_size() / 2;
	switch (pDirection)
	{
	case direction::other: return{ pos, {32, 32} };
	case direction::left:  return{ pos - engine::fvector(hsize.x, hsize.y / 2), { hsize.x, hsize.y / 2 } };
	case direction::right: return{ pos - engine::fvector(0, hsize.y / 2),{ hsize.x, hsize.y / 2 } };
	case direction::up:    return{ pos - engine::fvector(size.x / 2, hsize.y - 2), { size.x, hsize.y - 2 } };
	case direction::down:  return{ pos - engine::fvector(size.x / 2, 0),  { size.x, 4 } };
	}
	return{ 0, 0 };
}

void
player_character::movement(controls& pControls, collision_system& pCollision_system, float pDelta)
{
	if (mLocked)
	{
		stop_animation();
		return;
	}

	engine::fvector move(0, 0);

	if (pControls.is_triggered(controls::control::left))
	{
		if (!pCollision_system.wall_collision(get_collision(direction::left)))
			move.x -= 1;
		mFacing_direction = direction::left;
		set_cycle(character::e_cycle::left);
	}

	if (pControls.is_triggered(controls::control::right))
	{
		if (!pCollision_system.wall_collision(get_collision(direction::right)))
			move.x += 1;
		mFacing_direction = direction::right;
		set_cycle(character::e_cycle::right);
	}

	if (pControls.is_triggered(controls::control::up))
	{
		if (!pCollision_system.wall_collision(get_collision(direction::up)))
			move.y -= 1;
		mFacing_direction = direction::up;
		set_cycle(character::e_cycle::up);
	}

	if (pControls.is_triggered(controls::control::down))
	{
		if (!pCollision_system.wall_collision(get_collision(direction::down)))
			move.y += 1;
		mFacing_direction = direction::down;
		set_cycle(character::e_cycle::down);
	}

	if (move != engine::fvector(0, 0))
	{
		move.normalize();
		move *= get_speed();

		set_move_direction(move); // Make sure the player is in the direction he's moving
		set_position(get_position() + (move*pDelta));
		play_animation();
	}
	else
	{
		stop_animation();
	}
}

engine::fvector
player_character::get_activation_point(float pDistance)
{
	switch (mFacing_direction)
	{
	case direction::other: return get_position();
	case direction::left:  return get_position() + engine::fvector(-pDistance, 0);
	case direction::right: return get_position() + engine::fvector(pDistance, 0);
	case direction::up:    return get_position() + engine::fvector(0, -pDistance);
	case direction::down:  return get_position() + engine::fvector(0, pDistance);
	}
	return{ 0, 0 };
}

// #########
// entity_manager
// #########

entity_manager::entity_manager()
{
	add_child(mEntities);
	add_child(mCharacters);
}

character*
entity_manager::find_character(const std::string& pName)
{
	for (auto &i : mCharacters)
		if (i.get_name() == pName)
			return &i;
	return nullptr;
}

entity*
entity_manager::find_entity(const std::string& pName)
{
	for (auto &i : mEntities)
		if (i.get_name() == pName)
			return &i;
	return nullptr;
}

void entity_manager::clean()
{
	mCharacters.clear();
	mEntities.clear();
}

util::error
entity_manager::load_entities(tinyxml2::XMLElement * e)
{
	assert(mTexture_manager != nullptr);
	auto ele = e->FirstChildElement();
	while (ele)
	{
		std::string name = ele->Name();
		std::string att_texture = ele->Attribute("texture");
		engine::fvector pos = util::shortcuts::vector_float_att(ele)*32;

		auto& ne = mEntities.add_item();
		ne.load_entity(att_texture, *mTexture_manager);
		ne.set_position(pos);
		get_renderer()->add_client(ne);

		ele = ele->NextSiblingElement();
	}
	return 0;
}

void entity_manager::set_texture_manager(texture_manager& pTexture_manager)
{
	mTexture_manager = &pTexture_manager;
}

util::error
entity_manager::load_characters(tinyxml2::XMLElement * e)
{
	assert(mTexture_manager != nullptr);
	auto ele = e->FirstChildElement();
	while (ele)
	{
		std::string name = ele->Name();
		std::string att_texture = ele->Attribute("texture");
		engine::fvector pos = util::shortcuts::vector_float_att(ele);

		auto& ne = mCharacters.add_item();
		ne.load_entity(att_texture, *mTexture_manager);

		std::string att_cycle = util::safe_string(ele->Attribute("cycle"));
		ne.set_cycle(att_cycle.empty() ? "default" : att_cycle);

		ne.set_position(pos * 32);
		get_renderer()->add_client(ne);

		ele = ele->NextSiblingElement();
	}
	return 0;
}

entity* entity_manager::script_add_entity(const std::string & path)
{
	assert(get_renderer() != nullptr);
	assert(mTexture_manager != nullptr);

	if (mEntities.size() >= 256)
	{
		util::error("Reached upper limit of entities");
		return nullptr;
	}

	auto& ne = mEntities.add_item();
	ne.load_entity(path, *mTexture_manager);
	ne.set_animation("default:default");
	get_renderer()->add_client(ne);
	return &ne;
}

entity* entity_manager::script_add_entity_atlas(const std::string & path, const std::string& atlas)
{
	entity* e = script_add_entity(path);
	e->set_animation(atlas);
	return e;
}

void rpg::entity_manager::script_remove_entity(entity * e)
{
	assert(e != nullptr);
	if (is_character(e))
	{
		character* c = dynamic_cast<character*>(e);
		mCharacters.remove_item(c);
	}
	else
	{
		mEntities.remove_item(e);
	}
}

entity* entity_manager::script_add_character(const std::string & path)
{
	assert(get_renderer() != nullptr);
	assert(mTexture_manager != nullptr);

	if (mCharacters.size() >= 126)
	{
		util::error("Reached upper limit of characters.");
		return nullptr;
	}

	auto& nc = mCharacters.add_item();
	nc.load_entity(path, *mTexture_manager);
	nc.set_cycle(character::e_cycle::def);
	get_renderer()->add_client(nc);
	return &nc;
}

void entity_manager::script_set_name(entity * e, const std::string & pName)
{
	assert(e != nullptr);
	e->set_name(pName);
}

void entity_manager::script_set_position(entity* e, const engine::fvector & pos)
{
	assert(e != nullptr);
	e->set_position(pos*32);
}

engine::fvector entity_manager::script_get_position(entity * e)
{
	assert(e != nullptr);
	return e->get_position()/32;
}

void entity_manager::script_set_direction(entity* e, int dir)
{
	assert(e != nullptr);
	character* c = dynamic_cast<character*>(e);
	if (c == nullptr)
	{
		std::cout << "Error: Entity is not a character";
		return;
	}
	c->set_cycle(static_cast<character::e_cycle>(dir));
}

void entity_manager::script_set_cycle(entity* e, const std::string& name)
{
	assert(e != nullptr);
	character* c = dynamic_cast<character*>(e);
	if (c == nullptr)
	{
		std::cout << "Error: Entity is not a character";
		return;
	}
	c->set_cycle_group(name);
}

void entity_manager::script_set_depth(entity * e, float pDepth)
{
	assert(e != nullptr);
	e->set_depth(pDepth);
}

void entity_manager::script_set_depth_fixed(entity * e, bool pFixed)
{
	assert(e != nullptr);
	e->set_dynamic_depth(!pFixed);
}

void entity_manager::script_start_animation(entity* e)
{
	assert(e != nullptr);
	e->play_animation();
}

void entity_manager::script_stop_animation(entity* e)
{
	assert(e != nullptr);
	e->stop_animation();
}

void entity_manager::script_set_animation(entity* e, const std::string & name)
{
	assert(e != nullptr);
	e->set_animation(name);
}

void entity_manager::script_set_anchor(entity * e, int pAnchor)
{
	assert(e != nullptr);
	e->set_anchor(static_cast<engine::anchor>(pAnchor));
}

void entity_manager::script_set_rotation(entity * e, float pRotation)
{
	assert(e != nullptr);
	e->set_rotation(pRotation);
}

void entity_manager::script_set_color(entity * e, int r, int g, int b, int a)
{
	assert(e != nullptr);
	e->set_color(engine::color(r, g, b, a));
}

bool rpg::entity_manager::script_validate_entity(entity * e)
{
	for (auto& i : mCharacters)
	{
		if (&i == e)
			return true;
	}
	for (auto &i : mEntities)
	{
		if (&i == e)
			return true;
	}

	return false;
}

void entity_manager::load_script_interface(script_system& pScript)
{
	pScript.add_function("entity add_entity(const string &in)", asMETHOD(entity_manager, script_add_entity), this);
	pScript.add_function("entity add_entity(const string &in, const string &in)", asMETHOD(entity_manager, script_add_entity_atlas), this);
	pScript.add_function("entity add_character(const string &in)", asMETHOD(entity_manager, script_add_character), this);
	pScript.add_function("void set_position(entity, const vec &in)", asMETHOD(entity_manager, script_set_position), this);
	pScript.add_function("vec get_position(entity)", asMETHOD(entity_manager, script_get_position), this);
	pScript.add_function("void set_direction(entity, int)", asMETHOD(entity_manager, script_set_direction), this);
	pScript.add_function("void set_cycle(entity, const string &in)", asMETHOD(entity_manager, script_set_cycle), this);
	pScript.add_function("void start_animation(entity)", asMETHOD(entity_manager, script_start_animation), this);
	pScript.add_function("void stop_animation(entity)", asMETHOD(entity_manager, script_stop_animation), this);
	pScript.add_function("void set_animation(entity, const string &in)", asMETHOD(entity_manager, script_set_animation), this);
	pScript.add_function("entity find_entity(const string &in)", asMETHOD(entity_manager, find_entity), this);
	pScript.add_function("bool is_character(entity)", asMETHOD(entity_manager, is_character), this);
	pScript.add_function("void remove_entity(entity)", asMETHOD(entity_manager, script_remove_entity), this);
	pScript.add_function("void set_depth(entity, float)", asMETHOD(entity_manager, script_set_depth), this);
	pScript.add_function("void set_depth_fixed(entity, bool)", asMETHOD(entity_manager, script_set_depth_fixed), this);
	pScript.add_function("void set_name(entity, const string &in)", asMETHOD(entity_manager, script_set_name), this);
	pScript.add_function("void _set_anchor(entity, int)", asMETHOD(entity_manager, script_set_anchor), this);
	pScript.add_function("void set_rotation(entity, float)", asMETHOD(entity_manager, script_set_rotation), this);
	pScript.add_function("void set_color(entity, int, int, int, int)", asMETHOD(entity_manager, script_set_color), this);
	pScript.add_function("bool _validate_entity(entity)", asMETHOD(entity_manager, script_validate_entity), this);
}

bool entity_manager::is_character(entity* pEntity)
{
	return dynamic_cast<character*>(pEntity) != nullptr;
}

// #########
// scene
// #########

scene::scene()
{
	mTilemap_display.set_depth(defs::TILES_DEPTH);
	add_child(mTilemap_display);
	add_child(mEntity_manager);
	add_child(mPlayer);
	mFocus_player = true;
}

collision_system&
scene::get_collision_system()
{
	return mCollision_system;
}


void
scene::clean_scene(bool pFull)
{
	mScript->about_all();

	mTilemap_display.clean();
	mTilemap_loader.clean();
	mCollision_system.clean();
	mEntity_manager.clean();
	mNarrative.hide_box();
	mSound_FX.stop_all();

	focus_player(true);

	mPlayer.set_color({ 255, 255, 255, 255 });
	mPlayer.set_position({ 0, 0 });
	mPlayer.set_locked(false);
	mPlayer.set_cycle_group("default");
	mPlayer.set_cycle("default");
	mPlayer.set_dynamic_depth(true);
	mPlayer.set_rotation(0);

	if (pFull)
	{
		mBackground_music.clean();
	}
}


util::error scene::load_scene(std::string pPath)
{
	assert(mTexture_manager != nullptr);
	assert(mScript != nullptr);

	clean_scene();

	scene_loader loader;
	if (loader.load(pPath))
		return util::error("Unable to open scene");

	mScene_path = pPath;
	mScene_name = loader.get_name();

	mCollision_system.load_collision_boxes(loader.get_collisionboxes());

	set_boundary(loader.get_boundary());

	if (!mScript->load_scene_script(loader.get_script_path()))
		mScript->setup_triggers(mCollision_system);

	auto tilemap_texture = mTexture_manager->get_texture(loader.get_tilemap_texture());
	if (!tilemap_texture)
		return util::error("Invalid tilemap texture");
	mTilemap_display.set_texture(*tilemap_texture);

	mTilemap_loader.load_tilemap_xml(loader.get_tilemap());
	mTilemap_loader.update_display(mTilemap_display);

	// Pre-execute so the scene script can setup things before the render.
	mScript->tick();
	set_focus(mPlayer.get_position());

	return 0;
}

util::error
scene::reload_scene()
{
	if (mScene_path.empty())
		return util::error("No scene to reload");
	mBackground_music.clean();
	return load_scene(mScene_path);
}

void scene::load_script_interface(script_system& pScript)
{
	mNarrative.load_script_interface(pScript);
	mEntity_manager.load_script_interface(pScript);
	mBackground_music.load_script_interface(pScript);

	pScript.add_function("void set_tile(const string &in, vec, int, int)", asMETHOD(scene, script_set_tile), this);
	pScript.add_function("void remove_tile(vec, int)", asMETHOD(scene, script_remove_tile), this);

	pScript.add_function("int _spawn_sound(const string&in)", asMETHOD(sound_manager, spawn_sound), &mSound_FX);
	pScript.add_function("void _stop_all()", asMETHOD(sound_manager, stop_all), &mSound_FX);
	
	pScript.add_function("entity get_player()", asMETHOD(scene, script_get_player), this);
	pScript.add_function("void _lockplayer(bool)", asMETHOD(player_character, set_locked), &mPlayer);

	pScript.add_function("void set_focus(vec)", asMETHOD(scene, script_set_focus), this);
	pScript.add_function("vec get_focus()", asMETHOD(scene, script_get_focus), this);
	pScript.add_function("void focus_player(bool)", asMETHOD(scene, focus_player), this);

	mScript = &pScript;
}

void scene::set_texture_manager(texture_manager& pTexture_manager)
{
	mTexture_manager = &pTexture_manager;
	mEntity_manager.set_texture_manager(pTexture_manager);
}

void scene::load_game_xml(tinyxml2::XMLElement * ele_root)
{
	assert(ele_root != nullptr);

	auto ele_player = ele_root->FirstChildElement("player");
	if (!ele_player)
	{
		util::error("Please specify the player");
		return;
	}
	std::string att_texture = util::safe_string(ele_player->Attribute("texture"));

	mPlayer.load_entity(att_texture, *mTexture_manager);
	mPlayer.set_cycle(character::e_cycle::def);

	if (auto ele_sounds = ele_root->FirstChildElement("sounds"))
	{
		mSound_FX.load_sounds(ele_sounds);
	}

	if (auto ele_narrative = ele_root->FirstChildElement("narrative"))
	{
		assert(mTexture_manager != nullptr);
		mNarrative.load_narrative_xml(ele_narrative, *mTexture_manager);
	}
}

void
scene::tick(controls &pControls)
{
	assert(get_renderer() != nullptr);

	mPlayer.movement(pControls, mCollision_system, get_renderer()->get_delta());
	if (mFocus_player)
		set_focus(mPlayer.get_position());

	{
		auto pos = mPlayer.get_position();
		auto trigger = mCollision_system.trigger_collision(pos);
		if (trigger)
		{
			if (trigger->get_function().call())
				trigger->get_function().set_arg(0, &pos);
		}
	}

	{
		auto pos = mPlayer.get_position();
		auto door = mCollision_system.door_collision(pos);
		if (door)
		{
			std::string destination = door->destination;
			load_scene(door->scene_path);
			auto nposition = mCollision_system.get_door_entry(destination);
			mPlayer.set_position(nposition);
		}
	}

	if (pControls.is_triggered(controls::control::activate))
	{
		auto pos = mPlayer.get_activation_point();
		auto button = mCollision_system.button_collision(pos);
		if (button)
		{
			if (button->get_function().call())
				button->get_function().set_arg(0, &pos);
		}
	}
}

void scene::focus_player(bool pFocus)
{
	mFocus_player = pFocus;
}

void scene::script_set_focus(engine::fvector pPosition)
{
	mFocus_player = false;
	set_focus(pPosition * 32);
}

engine::fvector scene::script_get_focus()
{
	return get_focus() / 32;
}

entity* scene::script_get_player()
{
	return &mPlayer;
}

void scene::script_set_tile(const std::string& pAtlas, engine::fvector pPosition
	, int pLayer, int pRotation)
{
	mTilemap_loader.set_tile(pPosition, pLayer, pAtlas, pRotation);
	mTilemap_loader.update_display(mTilemap_display);
}

void scene::script_remove_tile(engine::fvector pPosition, int pLayer)
{
	mTilemap_loader.remove_tile(pPosition, pLayer);
	mTilemap_loader.update_display(mTilemap_display);
}

void scene::refresh_renderer(engine::renderer& pR)
{
	set_viewport(pR.get_size());
	pR.add_client(mTilemap_display);
	pR.add_client(mNarrative);
	pR.add_client(mPlayer);
	mEntity_manager.set_renderer(pR);
}


// #########
// controls
// #########

controls::controls()
{
	reset();
}

void
controls::trigger(control pControl)
{
	mControls[(int)pControl] = true;
}

bool
controls::is_triggered(control pControl)
{
	return mControls[(int)pControl];
}

void
controls::reset()
{
	mControls.assign(false);
}

// #########
// angelscript
// #########

void script_system::message_callback(const asSMessageInfo * msg)
{
	if (!mLog_file.is_open())
	{
		mLog_file.open("./data/log.txt");
	}

	std::string type = "ERROR";
	if (msg->type == asEMsgType::asMSGTYPE_INFORMATION)
		type = "INFO";
	else if(msg->type == asEMsgType::asMSGTYPE_WARNING)
		type = "WARNING";
	std::string message = msg->section;
	message += "( ";
	message += std::to_string(msg->row);
	message += ", ";
	message += std::to_string(msg->col);
	message += " ) : ";
	message += type;
	message += " : ";
	message += msg->message;
	message += "\n";
	std::cout << message;

	if (mLog_file.is_open())
		mLog_file << message;
}

std::string
script_system::get_metadata_type(const std::string & pMetadata)
{
	for (auto i = pMetadata.begin(); i != pMetadata.end(); i++)
	{
		if (!parsers::is_letter(*i))
			return std::string(pMetadata.begin(), i);
	}
	return pMetadata;
}

void script_system::script_abort()
{
	auto c_ctx = mCtxmgr.GetCurrentContext();
	assert(c_ctx != nullptr);
	c_ctx->Abort();
}


void
script_system::dprint(std::string &msg)
{
	std::cout << "Debug : " << msg << "\n";
}

void
script_system::register_vector_type()
{
	mEngine->RegisterObjectType("vec", sizeof(engine::fvector), asOBJ_VALUE | asGetTypeTraits<engine::fvector>());

	// Constructors and deconstructors
	mEngine->RegisterObjectBehaviour("vec", asBEHAVE_CONSTRUCT, "void f()"
		, asFUNCTION(script_default_constructor<engine::fvector>)
		, asCALL_CDECL_OBJLAST);
	mEngine->RegisterObjectBehaviour("vec", asBEHAVE_CONSTRUCT, "void f(float, float)"
		, asFUNCTIONPR(script_constructor<engine::fvector>, (float, float, void*), void)
		, asCALL_CDECL_OBJLAST);
	mEngine->RegisterObjectBehaviour("vec", asBEHAVE_CONSTRUCT, "void f(const vec&in)"
		, asFUNCTIONPR(script_constructor<engine::fvector>, (const engine::fvector&, void*), void)
		, asCALL_CDECL_OBJLAST);
	mEngine->RegisterObjectBehaviour("vec", asBEHAVE_DESTRUCT, "void f()"
		, asFUNCTION(script_default_deconstructor<engine::fvector>), asCALL_CDECL_OBJLAST);

	// Assignments
	mEngine->RegisterObjectMethod("vec", "vec& opAssign(const vec &in)"
		, asMETHODPR(engine::fvector, operator=, (const engine::fvector&), engine::fvector&)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec& opAddAssign(const vec &in)"
		, asMETHODPR(engine::fvector, operator+=<float>, (const engine::fvector&), engine::fvector&)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec& opSubAssign(const vec &in)"
		, asMETHODPR(engine::fvector, operator-=<float>, (const engine::fvector&), engine::fvector&)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec& opMulAssign(const vec &in)"
		, asMETHODPR(engine::fvector, operator*=<float>, (const engine::fvector&), engine::fvector&)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec& opMulAssign(float)"
		, asMETHODPR(engine::fvector, operator*=, (float), engine::fvector&)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec& opDivAssign(float)"
		, asMETHODPR(engine::fvector, operator/=, (float), engine::fvector&)
		, asCALL_THISCALL);

	// Arithmic
	mEngine->RegisterObjectMethod("vec", "vec opAdd(const vec &in) const"
		, asMETHODPR(engine::fvector, operator+<float>, (const engine::fvector&) const, engine::fvector)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec opSub(const vec &in) const"
		, asMETHODPR(engine::fvector, operator-<float>, (const engine::fvector&) const, engine::fvector)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec opMul(const vec &in) const"
		, asMETHODPR(engine::fvector, operator*<float>, (const engine::fvector&) const, engine::fvector)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec opMul(float) const"
		, asMETHODPR(engine::fvector, operator*<float>, (float) const, engine::fvector)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec opDiv(float) const"
		, asMETHODPR(engine::fvector, operator/<float>, (float) const, engine::fvector)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec opNeg() const"
		, asMETHODPR(engine::fvector, operator-, () const, engine::fvector)
		, asCALL_THISCALL);

	// Distance
	mEngine->RegisterObjectMethod("vec", "float distance() const"
		, asMETHODPR(engine::fvector, distance, () const, float)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "float distance(const vec &in) const"
		, asMETHODPR(engine::fvector, distance, (const engine::fvector&) const, float)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "float manhattan() const"
		, asMETHODPR(engine::fvector, manhattan, () const, float)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "float manhattan(const vec &in) const"
		, asMETHODPR(engine::fvector, manhattan, (const engine::fvector&) const, float)
		, asCALL_THISCALL);

	// Rotate
	mEngine->RegisterObjectMethod("vec", "vec& rotate(float)"
		, asMETHODPR(engine::fvector, rotate, (float), engine::fvector&) 
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec& rotate(const vec &in, float)"
		, asMETHODPR(engine::fvector, rotate, (const engine::fvector&, float), engine::fvector&)
		, asCALL_THISCALL);

	mEngine->RegisterObjectMethod("vec", "vec& normalize()"
		, asMETHOD(engine::fvector, normalize)
		, asCALL_THISCALL);

	mEngine->RegisterObjectMethod("vec", "vec& floor()"
		, asMETHOD(engine::fvector, floor)
		, asCALL_THISCALL);

	mEngine->RegisterObjectMethod("vec", "float angle() const"
		, asMETHOD(engine::fvector, angle)
		, asCALL_THISCALL);

	// Members
	mEngine->RegisterObjectProperty("vec", "float x", asOFFSET(engine::fvector, x));
	mEngine->RegisterObjectProperty("vec", "float y", asOFFSET(engine::fvector, y));
}

script_system::script_system()
{
	mEngine = asCreateScriptEngine();

	mEngine->SetEngineProperty(asEP_REQUIRE_ENUM_SCOPE, true);
	//mEngine->SetEngineProperty(asEP_BUILD_WITHOUT_LINE_CUES, true);

	mEngine->SetMessageCallback(asMETHOD(script_system, message_callback), this, asCALL_THISCALL);

	RegisterStdString(mEngine);
	RegisterScriptMath(mEngine);
	RegisterScriptArray(mEngine, true);
	mCtxmgr.RegisterCoRoutineSupport(mEngine);

	register_vector_type();

	add_function("int rand()", asFUNCTION(std::rand));
	add_function("void _timer_start(float)",  asMETHOD(engine::timer, start_timer), &mTimer);
	add_function("bool _timer_reached()",     asMETHOD(engine::timer, is_reached), &mTimer);
	add_function("void cocall(const string &in)", asMETHOD(script_system, call_event_function), this);
	add_function("void dprint(const string &in)", asMETHOD(script_system, dprint), this);
	add_function("void abort()", asMETHOD(script_system, script_abort), this);

	mExecuting = false;
}

script_system::~script_system()
{
	mCtxmgr.AbortAll();
	mCtxmgr.~CContextMgr(); // Destroy context manager before releasing engine
	mEngine->ShutDownAndRelease();
}

util::error
script_system::load_scene_script(const std::string& pPath)
{
	mCtxmgr.AbortAll();
	mEngine->DiscardModule(pPath.c_str());
	mBuilder.StartNewModule(mEngine, pPath.c_str());
	mBuilder.AddSectionFromMemory("scene_commands", "#include 'data/internal/scene.as'");
	mBuilder.AddSectionFromFile(pPath.c_str());
	if (mBuilder.BuildModule())
		return "Failed to load scene script";

	mScene_module = mBuilder.GetModule();

	start_all_with_tag("start");
	return 0;
}

void
script_system::add_function(const char* pDeclaration, const asSFuncPtr& pPtr, void* pInstance)
{
	int r = mEngine->RegisterGlobalFunction(pDeclaration, pPtr, asCALL_THISCALL_ASGLOBAL, pInstance);
	assert(r >= 0);
}

void
script_system::add_function(const char * pDeclaration, const asSFuncPtr& pPtr)
{
	int r = mEngine->RegisterGlobalFunction(pDeclaration, pPtr, asCALL_CDECL);
	assert(r >= 0);
}

void
script_system::add_pointer_type(const char* pName)
{
	mEngine->RegisterObjectType(pName, sizeof(void*), asOBJ_VALUE | asOBJ_APP_PRIMITIVE | asOBJ_POD);
}

void
script_system::call_event_function(const std::string& pName)
{
	auto func = mScene_module->GetFunctionByName(pName.c_str());
	if (!func)
		util::error("Function '" + pName + "' does not exist");
	mCtxmgr.AddContext(mEngine, func);
}

void
script_system::setup_triggers(collision_system& pCollision_system)
{
	size_t func_count = mScene_module->GetFunctionCount();
	for (size_t i = 0; i < func_count; i++)
	{
		auto func = mScene_module->GetFunctionByIndex(i);
		std::string metadata = mBuilder.GetMetadataStringForFunc(func);
		std::string type = get_metadata_type(metadata);

		if (type == "trigger" ||
			type == "button")
		{
			trigger nt;
			script_function& sfunc = nt.get_function();
			sfunc.set_context_manager(&mCtxmgr);
			sfunc.set_engine(mEngine);
			sfunc.set_function(func);

			std::string vectordata(metadata.begin() + type.length(), metadata.end());
			nt.parse_function_metadata(metadata);

			if (type == "trigger")
				pCollision_system.add_trigger(nt);
			if (type == "button")
				pCollision_system.add_button(nt);
		}
	}
}

void script_system::about_all()
{
	if (mCtxmgr.GetCurrentContext())
		mCtxmgr.PreAbout();
	else
		mCtxmgr.AbortAll();
}

void script_system::start_all_with_tag(const std::string & tag)
{
	size_t func_count = mScene_module->GetFunctionCount();
	for (size_t i = 0; i < func_count; i++)
	{
		auto func = mScene_module->GetFunctionByIndex(i);
		std::string metadata = mBuilder.GetMetadataStringForFunc(func);
		if (metadata == tag)
		{
			mCtxmgr.AddContext(mEngine, func);
		}
	}
}

int
script_system::tick()
{
	mExecuting = true;
	mCtxmgr.ExecuteScripts();
	mExecuting = false;
	return 0;
}

bool script_system::is_executing()
{
	return mExecuting;
}

// #########
// game
// #########

game::game()
{
	load_script_interface();
	mEditor_manager.set_texture_manager(mTexture_manager);
	mSlot = 0;
}

std::string game::get_slot_path(size_t pSlot)
{
	return "./data/saves/slot_"
		+ std::to_string(pSlot)
		+ ".xml";
}

void game::save_game(size_t pSlot)
{
	const std::string path = get_slot_path(pSlot);
	save_system file;
	file.new_save();
	file.save_flags(mFlags);
	file.save_scene(mScene);
	file.save_player(mScene.get_player());
	file.save(path);
	mSlot = pSlot;
}

void game::open_game(size_t pSlot)
{
	const std::string path = get_slot_path(pSlot);
	save_system file;
	if (!file.open_save(path))
	{
		util::error("Invalid slot");
		return;
	}
	file.load_flags(mFlags);
	file.load_player(mScene.get_player());
	mScript.about_all();
	if (mScript.is_executing())
	{
		mRequest_load = true;
		mNew_scene_path = file.get_scene_path();
	}
	else
	{
		mScene.load_scene(file.get_scene_path());
	}
}

bool game::is_slot_used(size_t pSlot)
{
	const std::string path = get_slot_path(pSlot);
	std::ifstream stream(path.c_str());
	return stream.good();
}

void game::script_load_scene(const std::string & pPath)
{
	mRequest_load = true;
	mNew_scene_path = pPath;
}

void
game::load_script_interface()
{
	mScript.add_pointer_type("entity");

	mScript.add_function("float get_delta()", asMETHOD(game, get_delta), this);

	mScript.add_function("bool _is_triggered(int)", asMETHOD(controls, is_triggered), &mControls);

	mScript.add_function("void save_game(uint)", asMETHOD(game, save_game), this);
	mScript.add_function("void open_game(uint)", asMETHOD(game, open_game), this);
	mScript.add_function("bool is_slot_used(uint)", asMETHOD(game, is_slot_used), this);
	mScript.add_function("void load_scene(const string &in)", asMETHOD(game, script_load_scene), this);

	mFlags.load_script_interface(mScript);
	mScene.load_script_interface(mScript);
	mValue_manager.load_script_interface(mScript);
}

float game::get_delta()
{
	return get_renderer()->get_delta();
}

util::error
game::load_game_xml(std::string pPath)
{
	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
		return "Could not load game file at '" + pPath + "'";

	auto ele_root = doc.RootElement();

	auto ele_scene = ele_root->FirstChildElement("scene");
	if (!ele_scene)
		return "Please specify the scene to start with";
	std::string scene_path = util::safe_string(ele_scene->Attribute("path"));

	auto ele_textures = ele_root->FirstChildElement("textures");
	if (!ele_textures)
		return "Please specify the texture file";
	mTexture_manager.load_settings(ele_textures);

	mScene.set_texture_manager(mTexture_manager);
	mScene.load_game_xml(ele_root);
	mScene.load_scene(scene_path);
	return 0;
}

void
game::tick(controls& pControls)
{
	if (pControls.is_triggered(controls::control::reset))
	{
		mEditor_manager.close_editor();
		std::cout << "Reloading scene...\n";
		mScene.reload_scene();
		std::cout << "Done\n";
	}

	mControls = pControls;

	if (pControls.is_triggered(controls::control::editor_1))
	{
		mEditor_manager.close_editor();
		mEditor_manager.open_tilemap_editor(mScene.get_path());
		mScene.clean_scene(true);
	}

	if (pControls.is_triggered(controls::control::editor_2))
	{
		mEditor_manager.close_editor();
		mEditor_manager.open_collisionbox_editor(mScene.get_path());
		mScene.clean_scene(true);
	}
	if (mEditor_manager.is_editor_open())
		return;

	mScene.tick(pControls);

	mScript.tick();
	if (mRequest_load)
	{
		mRequest_load = false;
		mScene.load_scene(mNew_scene_path);
	}

	mEditor_manager.update_camera_position(mScene.get_exact_position());
}

void
game::refresh_renderer(engine::renderer & pR)
{
	mScene.set_renderer(pR);
	pR.add_client(mEditor_manager);
	pR.set_icon("data/icon.png");
}

// ##########
// panning_node
// ##########

void
panning_node::set_boundary(engine::fvector pBoundary)
{
	mBoundary = pBoundary;
}

void
panning_node::set_viewport(engine::fvector pViewport)
{
	mViewport = pViewport;
}

void
panning_node::set_focus(engine::fvector pFocus)
{
	engine::fvector offset = pFocus - mViewport * 0.5f;
	if (mBoundary.x < mViewport.x)
		offset.x = (mBoundary.x * 0.5f) - (mViewport.x * 0.5f);
	else
		offset.x = util::clamp(offset.x, 0.f, mBoundary.x - mViewport.x);

	if (mBoundary.y < mViewport.y)
		offset.y = (mBoundary.y * 0.5f) - (mViewport.y * 0.5f);
	else
		offset.y = util::clamp(offset.y, 0.f, mBoundary.y - mViewport.y);

	set_position(-offset);
	mFocus = pFocus;
}

engine::fvector panning_node::get_focus()
{
	return mFocus;
}

// ##########
// narrative_dialog
// ##########

util::error
narrative_dialog::load_box(tinyxml2::XMLElement* pEle, texture_manager& pTexture_manager)
{
	auto ele_box = pEle->FirstChildElement("box");

	std::string att_box_tex = ele_box->Attribute("texture");
	std::string att_box_atlas = ele_box->Attribute("atlas");

	auto tex = pTexture_manager.get_texture(att_box_tex);
	if (!tex) return "Texture does not exist";
	mBox.set_texture(*tex, att_box_atlas);

	set_box_position(position::bottom);

	mSelection.set_position(mBox.get_size() - engine::fvector(20, 10));
	return 0;
}

util::error
narrative_dialog::load_font(tinyxml2::XMLElement* pEle)
{
	auto ele_font = pEle->FirstChildElement("font");

	std::string att_font_path = ele_font->Attribute("path");

	mFont.load(att_font_path);
	mText.set_font(mFont);
	mText.set_character_size(ele_font->IntAttribute("size"));
	mText.set_scale(ele_font->FloatAttribute("scale"));

	mSelection.copy_format(mText);
	return 0;
}

void narrative_dialog::show_expression()
{
	mExpression.set_visible(true);
	engine::fvector position =
		mExpression.get_position()
		+ engine::fvector(mExpression.get_size().x, 0)
		+ engine::fvector(10, 10);
	mText.set_position(position);
}

void narrative_dialog::reset_positions()
{
	set_box_position(position::bottom);
	mText.set_position({ 10, 10 });
}

narrative_dialog::narrative_dialog()
{
	mRevealing = false;
	hide_box();
	mInterval = defs::DEFAULT_DIALOG_SPEED;

	mBox.set_depth(defs::NARRATIVE_BOX_DEPTH);

	mText.set_depth(defs::NARRATIVE_TEXT_DEPTH);
	mBox.add_child(mText);

	mSelection.set_depth(defs::NARRATIVE_TEXT_DEPTH);
	mBox.add_child(mSelection);

	mExpression.set_depth(defs::NARRATIVE_TEXT_DEPTH);
	mBox.add_child(mExpression);

	mExpression.set_position({ 10, 10 });

	mSelection.set_anchor(engine::anchor::bottomright);

	reset_positions();
}

void
narrative_dialog::set_box_position(position pPosition)
{
	float offx = (defs::DISPLAY_SIZE.x - mBox.get_size().x) / 2;

	switch (pPosition)
	{
	case position::top:
	{
		mBox.set_position({ offx, 10 });
		break;
	}
	case position::bottom:
	{
		mBox.set_position({ offx, defs::DISPLAY_SIZE.y - mBox.get_size().y - 10 });
		break;
	}
	}
}

bool
narrative_dialog::is_revealing()
{
	return mRevealing;
}

void narrative_dialog::reveal_text(const std::string& pText, bool pAppend)
{
	mTimer.restart();
	mRevealing = true;

	if (pAppend)
	{
		mFull_text += pText;
	}
	else
	{
		mFull_text = pText;
		mText.set_text("");
		mCount = 0;
	}
}

void narrative_dialog::instant_text(std::string pText, bool pAppend)
{
	mRevealing = false;
	if (pAppend)
		mFull_text += pText;
	else
		mFull_text = pText;
	mText.set_text(mFull_text);
}

void narrative_dialog::show_box()
{
	mText.set_visible(true);
	mBox.set_visible(true);
}

void narrative_dialog::hide_box()
{
	mRevealing = false;
	mText.set_visible(false);
	mText.set_text("");
	mBox.set_visible(false);
	mSelection.set_visible(false);
	mExpression.set_visible(false);
	reset_positions();
	set_interval(defs::DEFAULT_DIALOG_SPEED);
}

bool narrative_dialog::is_box_open()
{
	return mBox.is_visible();
}

void narrative_dialog::set_interval(float ms)
{
	mInterval = ms;
}

void narrative_dialog::show_selection()
{
	if (mBox.is_visible())
	{
		mSelection.set_visible(true);
	}
}

void narrative_dialog::hide_selection()
{
	mSelection.set_visible(false);
}

void narrative_dialog::set_selection(const std::string& pText)
{
	mSelection.set_text(pText);
}

void narrative_dialog::set_expression(const std::string& pName)
{
	auto animation = mExpression_manager.find_animation(pName);
	if (!animation)
	{
		util::error("Expression '" + pName + "' does not exist");
		return;
	}
	mExpression.set_animation(*animation);
	show_expression();
}

util::error narrative_dialog::load_narrative_xml(tinyxml2::XMLElement* pEle, texture_manager& pTexture_manager)
{
	load_box(pEle, pTexture_manager);
	load_font(pEle);

	if (auto ele_expressions = pEle->FirstChildElement("expressions"))
	{
		mExpression_manager.load_expressions_xml(ele_expressions, pTexture_manager);
	}
	return 0;
}

void narrative_dialog::load_script_interface(script_system & pScript)
{
	pScript.add_function("void _say(const string &in, bool)", asMETHOD(narrative_dialog, reveal_text), this);
	pScript.add_function("bool _is_revealing()", asMETHOD(narrative_dialog, is_revealing), this);
	pScript.add_function("void _showbox()", asMETHOD(narrative_dialog, show_box), this);
	pScript.add_function("void _hidebox()", asMETHOD(narrative_dialog, hide_box), this);
	pScript.add_function("void _show_selection()", asMETHOD(narrative_dialog, show_selection), this);
	pScript.add_function("void _hide_selection()", asMETHOD(narrative_dialog, hide_selection), this);
	pScript.add_function("void _set_interval(float)", asMETHOD(narrative_dialog, set_interval), this);
	pScript.add_function("void _set_box_position(int)", asMETHOD(narrative_dialog, set_box_position), this);
	pScript.add_function("void _set_selection(const string &in)", asMETHOD(narrative_dialog, set_selection), this);

	pScript.add_function("void _set_expression(const string &in)", asMETHOD(narrative_dialog, set_expression), this);
	pScript.add_function("void _start_expression_animation()", asMETHOD(engine::animation_node, start), &mExpression);
	pScript.add_function("void _stop_expression_animation()", asMETHOD(engine::animation_node, stop), &mExpression);
}

int narrative_dialog::draw(engine::renderer& pR)
{
	if (!mRevealing) return 0;

	float time = mTimer.get_elapse().ms();
	if (time >= mInterval)
	{
		mCount += static_cast<size_t>(time / mInterval);
		mCount  = util::clamp<size_t>(mCount, 0, mFull_text.size());

		std::string display(mFull_text.begin(), mFull_text.begin() + mCount);
		mText.set_text(display);

		if (mCount >= mFull_text.size())
			mRevealing = false;

		mTimer.restart();
	}
	return 0;
}

void narrative_dialog::refresh_renderer(engine::renderer& r)
{
	r.add_client(mBox);
	r.add_client(mText);
	r.add_client(mSelection);
	r.add_client(mExpression);
}

// ##########
// script_function
// ##########

script_function::script_function() :
	as_engine(nullptr),
	func(nullptr),
	ctx(nullptr),
	func_ctx(nullptr)
{
}

script_function::~script_function()
{
	//return_context();
}

bool
script_function::is_running()
{
	if (!func_ctx)
		return false;
	if (func_ctx->GetState() == AS::asEXECUTION_FINISHED)
		return false;
	return true;
}

void
script_function::set_engine(AS::asIScriptEngine * e)
{
	as_engine = e;
}

void
script_function::set_function(AS::asIScriptFunction * f)
{
	func = f;
}

void
script_function::set_context_manager(AS::CContextMgr * cm)
{
	ctx = cm;
}

void
script_function::set_arg(unsigned int index, void* ptr)
{
	assert(func_ctx != nullptr);
	assert(func != nullptr);
	if(index < func->GetParamCount())
		func_ctx->SetArgObject(index, ptr);
}

bool
script_function::call()
{
	assert(as_engine != nullptr);
	assert(func != nullptr);
	assert(ctx != nullptr);
	if (!is_running())
	{
		return_context();
		func_ctx = ctx->AddContext(as_engine, func, true);
		return true;
	}
	return false;
}

void script_function::return_context()
{
	if (func_ctx)
	{
		func_ctx->Abort();
		ctx->DoneWithContext(func_ctx);
		func_ctx = nullptr;
	}
}

// ##########
// tilemap_loader
// ##########

void
tilemap_loader::tile::load_xml(tinyxml2::XMLElement * e, size_t _layer)
{
	assert(e != nullptr);

	atlas = util::safe_string(e->Name());

	position.x = e->FloatAttribute("x");
	position.y = e->FloatAttribute("y");

	fill.x = e->IntAttribute("w");
	fill.y = e->IntAttribute("h");
	fill.x = fill.x <= 0 ? 1 : fill.x; // Default 1
	fill.y = fill.y <= 0 ? 1 : fill.y;

	rotation = e->IntAttribute("r") % 4;
}

bool
tilemap_loader::tile::is_adjacent_above(tile & a)
{
	return (
		atlas == a.atlas
		&& position.x == a.position.x
		&& position.y == a.position.y + a.fill.y
		&& fill.x == a.fill.x
		);
}

bool
tilemap_loader::tile::is_adjacent_right(tile & a)
{
	return (
		atlas == a.atlas
		&& position.y == a.position.y
		&& position.x + fill.x == a.position.x
		&& fill.y == a.fill.y
		);
}

tilemap_loader::tile*
tilemap_loader::find_tile(engine::fvector pos, size_t layer)
{
	for (auto &i : mMap[layer])
	{
		if (i.second.position == pos)
			return &i.second;
	}
	return nullptr;
}

// Uses brute force to merge adjacent tiles (still a little wonky)
void
tilemap_loader::condense_layer(layer &pMap)
{
	if (pMap.size() < 2)
		return;

	layer nmap;

	tile new_tile = pMap.begin()->second;

	bool merged = false;
	for (auto &i = pMap.begin(); i != pMap.end(); i++)
	{
		auto& current_tile = i->second;

		// Merge adjacent tile 
		if (new_tile.is_adjacent_right(current_tile))
		{
			new_tile.fill.x += current_tile.fill.x;
		}
		else // No more tiles to the right
		{
			// Merge ntime to any tile above
			for (auto &j : nmap)
			{
				if (new_tile.is_adjacent_above(j.second))
				{
					j.second.fill.y += new_tile.fill.y;
					merged = true;
					break;
				}
			}
			if (!merged) // Do not add when it was merged to another tile
			{
				nmap[new_tile.position] = new_tile;
			}
			merged = false;
			new_tile = current_tile;
		}
	}
	nmap[new_tile.position] = new_tile; // add last tile
	pMap = std::move(nmap);
}

void
tilemap_loader::condense_tiles()
{
	if (!mMap.size()) return;
	for (auto &i : mMap)
	{
		condense_layer(i.second);
	}
}

util::error
tilemap_loader::load_layer(tinyxml2::XMLElement * pEle, int pLayer)
{
	auto i = pEle->FirstChildElement();
	while (i)
	{
		tile ntile;
		ntile.load_xml(i, pLayer);
		mMap[pLayer][ntile.position] = ntile;
		i = i->NextSiblingElement();
	}
	return 0;
}

tilemap_loader::tilemap_loader()
{
	mTile_size = defs::TILE_SIZE;
}

util::error
tilemap_loader::load_tilemap_xml(tinyxml2::XMLElement *pRoot)
{
	clean();

	if (auto att_path = pRoot->Attribute("path"))
	{
		load_tilemap_xml(util::safe_string(att_path));
	}

	auto ele_tilemap = pRoot->FirstChildElement("layer");
	while (ele_tilemap)
	{
		int att_layer = ele_tilemap->IntAttribute("id");
		load_layer(ele_tilemap, att_layer);
		ele_tilemap = ele_tilemap->NextSiblingElement("layer");
	}
	return 0;
}

util::error
tilemap_loader::load_tilemap_xml(std::string pPath)
{
	using namespace tinyxml2;
	XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
		return "Error loading tilemap file";
	auto root = doc.RootElement();
	load_tilemap_xml(root);
	return 0;
}

tilemap_loader::tile*
tilemap_loader::find_tile_at(engine::fvector pPosition, int pLayer)
{
	for (auto &i : mMap[pLayer])
	{
		auto& tile = i.second;
		if (engine::frect(tile.position, tile.fill).is_intersect(pPosition))
		{
			return &tile;
		}
	}
	return nullptr;
}

void tilemap_loader::explode_tile(tile* pTile, int pLayer)
{
	auto atlas = pTile->atlas;
	auto fill = pTile->fill;
	int rotation = pTile->rotation;
	auto position = pTile->position;
	pTile->fill = { 1, 1 };
	for (float x = 0; x < fill.x; x += 1)
		for (float y = 0; y < fill.y; y += 1)
			set_tile(position + engine::fvector(x, y), pLayer, atlas, rotation);
}

void
tilemap_loader::explode_tile(engine::fvector pPosition, int pLayer)
{
	auto t = find_tile_at(pPosition, pLayer);
	if (!t || t->fill == engine::fvector(1, 1))
		return;
	explode_tile(t, pLayer);
}

void tilemap_loader::explode_all()
{
	for (auto &l : mMap)
		for (auto& i : l.second)
			explode_tile(&i.second, l.first);
}

void
tilemap_loader::generate(tinyxml2::XMLDocument& doc, tinyxml2::XMLNode * root)
{
	std::map<int, tinyxml2::XMLElement *> layers;
	
	for (auto &l : mMap)
	{
		if (!l.second.size())
			continue;

		auto ele_layer = doc.NewElement("layer");
		ele_layer->SetAttribute("id", l.first);
		root->InsertEndChild(ele_layer);
		layers[l.first] = ele_layer;

		for (auto &i : l.second)
		{
			auto& tile = i.second;
			auto ele = doc.NewElement(tile.atlas.c_str());
			ele->SetAttribute("x", tile.position.x);
			ele->SetAttribute("y", tile.position.y);
			if (tile.fill.x > 1)    ele->SetAttribute("w", tile.fill.x);
			if (tile.fill.y > 1)    ele->SetAttribute("h", tile.fill.y);
			if (tile.rotation != 0) ele->SetAttribute("r", tile.rotation);
			layers[l.first]->InsertEndChild(ele);
		}
	}
}

void
tilemap_loader::generate(const std::string& pPath)
{
	using namespace tinyxml2;
	XMLDocument doc;
	auto root = doc.InsertEndChild(doc.NewElement("map"));
	generate(doc, root);
	doc.SaveFile(pPath.c_str());
}

int
tilemap_loader::set_tile(engine::fvector pPosition, engine::fvector pFill, int pLayer, const std::string& pAtlas, int pRotation)
{
	engine::fvector off(0, 0);
	for (off.y = 0; off.y <= pFill.y; off.y++)
	{
		for (off.x = 0; off.x <= pFill.x; off.x++)
		{
			set_tile(pPosition + off, pLayer, pAtlas, pRotation);
		}
	}
	return 0;
}

int
tilemap_loader::set_tile(engine::fvector pPosition, int pLayer, const std::string& pAtlas, int pRotation)
{
	auto &nt = mMap[pLayer][pPosition];
	nt.position = pPosition;
	nt.fill = { 1, 1 };
	nt.atlas = pAtlas;
	nt.rotation = pRotation;
	return 0;
}

void tilemap_loader::remove_tile(engine::fvector pPosition, int pLayer)
{
	for (auto &i = mMap[pLayer].begin(); i != mMap[pLayer].end(); i++)
	{
		if (i->second.position == pPosition)
		{
			mMap[pLayer].erase(i);
			break;
		}
	}
}

void
tilemap_loader::update_display(tilemap_display& tmA)
{
	tmA.clean();
	for (auto &l : mMap)
	{
		for (auto &i : l.second)
		{
			auto& tile = i.second;
			engine::fvector off(0, 0);
			for (off.y = 0; off.y < tile.fill.y; off.y++)
			{
				for (off.x = 0; off.x < tile.fill.x; off.x++)
				{
					tmA.set_tile((tile.position + off)*mTile_size, tile.atlas, l.first, tile.rotation);
				}
			}
		}
	}
}

void tilemap_loader::clean()
{
	mMap.clear();
}
/*
int tilemap_loader::draw(engine::renderer & r)
{
	engine::fvector pos = get_exact_position();

	engine::fvector mousepos = r.get_mouse_position(pos) / 32;
	mousepos.x = std::floor(mousepos.x);
	mousepos.y = std::floor(mousepos.y);

	if (r.is_mouse_pressed(engine::events::mouse_button::mouse_left))
	{
		break_tile(mousepos, 0);
		set_tile(mousepos, 0, "wall", 0);
		update_display();
	}
	if (r.is_mouse_pressed(engine::events::mouse_button::mouse_right))
	{
		remove_tile(mousepos, 0);
		update_display();
	}

	node.draw(r);
	return 0;
}*/

// ##########
// background_music
// ##########

void background_music::load_script_interface(script_system & pScript)
{
	pScript.add_function("void _music_play()", asMETHOD(engine::sound_stream, play), &mStream);
	pScript.add_function("void _music_stop()", asMETHOD(engine::sound_stream, stop), &mStream);
	pScript.add_function("void _music_pause()", asMETHOD(engine::sound_stream, pause), &mStream);
	pScript.add_function("float _music_position()", asMETHOD(engine::sound_stream, get_position), &mStream);
	pScript.add_function("void _music_volume(float)", asMETHOD(engine::sound_stream, set_volume), &mStream);
	pScript.add_function("void _music_set_loop(bool)", asMETHOD(engine::sound_stream, set_loop), &mStream);
	pScript.add_function("int _music_open(const string &in)", asMETHOD(background_music, script_music_open), this);
	pScript.add_function("bool _music_is_playing()", asMETHOD(engine::sound_stream, is_playing), &mStream);
	pScript.add_function("float _music_get_duration()", asMETHOD(engine::sound_stream, get_duration), &mStream);
}

void background_music::clean()
{
	mStream.stop();
}

int background_music::script_music_open(const std::string & pPath)
{
	if (mPath != pPath)
	{
		mPath = pPath;
		return mStream.open(pPath);
	}
	return 0;
}

// ##########
// collision_box
// ##########

collision_box::collision_box()
	: valid(true)
{
}

bool collision_box::is_valid()
{
	return valid;
}

void collision_box::validate(flag_container & pFlags)
{
	if (!mInvalid_on_flag.empty())
		valid = !pFlags.has_flag(mInvalid_on_flag);
	if (!mSpawn_flag.empty())
		pFlags.set_flag(mSpawn_flag);
}

void collision_box::load_xml(tinyxml2::XMLElement * e)
{
	mInvalid_on_flag = util::safe_string(e->Attribute("invalid"));
	mSpawn_flag = util::safe_string(e->Attribute("spawn"));

	engine::frect rect;
	rect.x = e->FloatAttribute("x");
	rect.y = e->FloatAttribute("y");
	rect.w = e->FloatAttribute("w");
	rect.h = e->FloatAttribute("h");
	set_rect(engine::scale(rect, defs::TILE_SIZE));
}

script_function& trigger::get_function()
{
	return mFunc;
}

void trigger::parse_function_metadata(const std::string & pMetadata)
{
	auto rect = parsers::parse_attribute_rect<float>(pMetadata);
	set_rect(engine::scale(rect, 32));
}

// ##########
// tilemap_display
// ##########

void tilemap_display::set_texture(engine::texture & pTexture)
{
	mTexture = &pTexture;
}

void tilemap_display::set_tile(engine::fvector pPosition, const std::string & pAtlas, int pLayer, int pRotation)
{
	assert(mTexture != nullptr);

	auto animation = mTexture->get_animation(pAtlas);
	if (animation == nullptr)
	{
		util::error("Tile not found");
		return;
	}

	auto &ntile = mLayers[pLayer].tiles[pPosition];
	ntile.mRef = mLayers[pLayer].vertices.add_quad(pPosition, animation->get_frame_at(0), pRotation);
	ntile.set_animation(animation);
	if (animation->get_frame_count() > 1)
		mAnimated_tiles.push_back(&ntile);
}

int tilemap_display::draw(engine::renderer& pR)
{
	if (!mTexture) return 1;
	update_animations();
	for (auto &i : mLayers)
	{
		auto& vb = i.second.vertices;
		vb.set_texture(*mTexture);
		vb.set_position(engine::fvector(get_exact_position()).round());
		vb.draw(pR);
	}
	return 0;
}

void tilemap_display::update_animations()
{
	try {
		for (auto i : mAnimated_tiles)
		{
			i->update_animation();
		}
	}
	catch (const std::exception& e)
	{
		util::error("Exception '" + std::string(e.what()) + "'");
		util::error("Failed to animate tiles");
		mAnimated_tiles.clear();
	}
}

void tilemap_display::clean()
{
	mLayers.clear();
	mAnimated_tiles.clear();
}

void tilemap_display::set_color(engine::color pColor)
{
	for (auto& l : mLayers)
	{
		l.second.vertices.set_color(pColor);
	}
}

void tilemap_display::highlight_layer(int pLayer, engine::color pHighlight, engine::color pOthers)
{
	for (auto& l : mLayers)
	{
		if (l.first == pLayer)
			l.second.vertices.set_color(pHighlight);
		else
			l.second.vertices.set_color(pOthers);
	}
}

void tilemap_display::remove_highlight()
{
	for (auto& l : mLayers)
	{
		l.second.vertices.set_color({255, 255, 255, 255});
	}
}

void tilemap_display::tile::set_animation(const engine::animation* pAnimation)
{
	mAnimation = pAnimation;
	mFrame = 0;
	mTimer.start_timer(pAnimation->get_interval()*0.001f);
}

void tilemap_display::tile::update_animation()
{
	if (!mAnimation) return;
	if (!mAnimation->get_frame_count()) return;
	if (mTimer.is_reached())
	{
		++mFrame;
		mTimer.start_timer(mAnimation->get_interval(mFrame)*0.001f);
		mRef.set_texture_rect(mAnimation->get_frame_at(mFrame), 0);
	}
}

// ##########
// save_system
// ##########

save_system::save_system()
{
	mEle_root = nullptr;
}

bool save_system::open_save(const std::string& pPath)
{
	mDocument.Clear();
	if (mDocument.LoadFile(pPath.c_str()))
	{
		return false;
	}
	mEle_root = mDocument.RootElement();
	return true;
}

void save_system::load_flags(flag_container& pFlags)
{
	assert(mEle_root != nullptr);
	auto ele_flag = mEle_root->FirstChildElement("flag");
	while (ele_flag)
	{
		pFlags.set_flag(util::safe_string(ele_flag->Attribute("name")));
		ele_flag = ele_flag->NextSiblingElement("flag");
	}
}

void save_system::load_player(player_character& pPlayer)
{
	assert(mEle_root != nullptr);
	auto ele_player = mEle_root->FirstChildElement("player");
	engine::fvector position;
	position.x = ele_player->FloatAttribute("x");
	position.y = ele_player->FloatAttribute("y");
	pPlayer.set_position(position);
}
std::string save_system::get_scene_path()
{
	assert(mEle_root != nullptr);
	auto ele_scene = mEle_root->FirstChildElement("scene");
	return util::safe_string(ele_scene->Attribute("path"));
}

std::string save_system::get_scene_name()
{
	assert(mEle_root != nullptr);
	auto ele_scene = mEle_root->FirstChildElement("scene");
	return util::safe_string(ele_scene->Attribute("name"));
}

void save_system::new_save()
{
	mDocument.Clear();
	mEle_root = mDocument.NewElement("save_file");
	mDocument.InsertEndChild(mEle_root);
}

void save_system::save(const std::string& pPath)
{
	assert(mEle_root != nullptr);
	mDocument.SaveFile(pPath.c_str());
}

void save_system::save_flags(flag_container& pFlags)
{
	assert(mEle_root != nullptr);
	for (auto &i : pFlags)
	{
		auto ele_flag = mDocument.NewElement("flag");
		ele_flag->SetAttribute("name", i.c_str());
		mEle_root->InsertEndChild(ele_flag);
	}
}

void save_system::save_scene(scene& pScene)
{
	assert(mEle_root != nullptr);
	auto ele_scene = mDocument.NewElement("scene");
	mEle_root->InsertFirstChild(ele_scene);
	ele_scene->SetAttribute("name", pScene.get_name().c_str());
	ele_scene->SetAttribute("path", pScene.get_path().c_str());
}

void save_system::save_player(player_character& pPlayer)
{
	assert(mEle_root != nullptr);
	auto ele_scene = mDocument.NewElement("player");
	mEle_root->InsertFirstChild(ele_scene);
	ele_scene->SetAttribute("x", pPlayer.get_position().x);
	ele_scene->SetAttribute("y", pPlayer.get_position().y);
}

// ##########
// expression_manager
// ##########

const engine::animation* expression_manager::find_animation(const std::string & mName)
{
	auto &find = mAnimations.find(mName);
	if (find != mAnimations.end())
	{
		return find->second;
	}
	return nullptr;
}

int expression_manager::load_expressions_xml(tinyxml2::XMLElement * pRoot, texture_manager & pTexture_manager)
{
	assert(pRoot != nullptr);
	auto ele_expression = pRoot->FirstChildElement();
	while (ele_expression)
	{
		const char* att_texture = ele_expression->Attribute("texture");
		if (!att_texture)
		{
			util::error("Please specify texture for expression");
			ele_expression = ele_expression->NextSiblingElement();
			continue;
		}
		auto texture = pTexture_manager.get_texture(att_texture);

		const char* att_atlas = ele_expression->Attribute("atlas");
		if (!att_atlas)
		{
			util::error("Please specify atlas for expression");
			ele_expression = ele_expression->NextSiblingElement();
			continue;
		}
		mAnimations[ele_expression->Name()] = texture->get_animation(att_atlas);

		ele_expression = ele_expression->NextSiblingElement();
	}
	return 0;
}

// ##########
// particle_manager
// ##########

void particle_manager::load_emitter_xml(tinyxml2::XMLElement* pEle)
{
	mEmitters.emplace_back();
	auto& emitter = mEmitters.back();
	emitter.set_acceleration(pEle->FloatAttribute("acceleration"));
}

// ##########
// save_value_manager
// ##########

void save_value_manager::set_value(const std::string & pName, const std::string & pVal)
{
	auto& val = mValues[pName];
	val.type = value_type::string;
	val.val_string = pVal;
}

void save_value_manager::set_value(const std::string & pName, int pVal)
{
	auto& val = mValues[pName];
	val.type = value_type::integer;
	val.val_integer = pVal;
}

void save_value_manager::set_value(const std::string & pName, float pVal)
{
	auto& val = mValues[pName];
	val.type = value_type::floating_point;
	val.val_floating_point = pVal;
}

const std::string& save_value_manager::get_value_string(const std::string& pName)
{
	auto& val = mValues.find(pName);
	if (val == mValues.end())
		return std::string(); // NEED TO FIX
	return val->second.val_string;
}

int save_value_manager::get_value_int(const std::string& pName)
{
	auto& val = mValues.find(pName);
	if (val == mValues.end())
		return 0;

	
	return val->second.val_integer;
}

float save_value_manager::get_value_float(const std::string & pName)
{
	auto& val = mValues.find(pName);
	if (val == mValues.end())
		return 0;
	return val->second.val_floating_point;
}

bool save_value_manager::remove_value(const std::string & pName)
{
	auto& val = mValues.find(pName);
	if (val == mValues.end())
		return false;
	mValues.erase(val);
	return true;
}

void save_value_manager::load_script_interface(script_system& pScript)
{
	pScript.add_function("void _set_value(const string &in, const string&in)", asMETHODPR(save_value_manager, set_value, (const std::string&, const std::string&), void), this);
	pScript.add_function("void _set_value(const string &in, int)", asMETHODPR(save_value_manager, set_value, (const std::string&, int), void), this);
	pScript.add_function("void _set_value(const string &in, float)", asMETHODPR(save_value_manager, set_value, (const std::string&, float), void), this);

	pScript.add_function("bool _remove_value(const string &in)", asMETHODPR(save_value_manager, remove_value, (const std::string&), bool), this);

	pScript.add_function("const string& _get_value_string(const string &in)", asMETHOD(save_value_manager, get_value_string), this);
	pScript.add_function("const string& _get_value_int(const string &in)", asMETHOD(save_value_manager, get_value_int), this);
	pScript.add_function("const string& _get_value_float(const string &in)", asMETHOD(save_value_manager, get_value_float), this);
}
