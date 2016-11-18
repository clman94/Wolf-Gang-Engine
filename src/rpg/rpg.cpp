#include <rpg/rpg.hpp>

#include <angelscript/add_on/scriptstdstring/scriptstdstring.h>
#include <angelscript/add_on/scriptmath/scriptmath.h>
#include <angelscript/add_on/scriptdictionary/scriptdictionary.h>

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
	return mFlags.emplace(pName).second;
}
bool flag_container::unset_flag(const std::string& pName)
{
	return mFlags.erase(pName) == 1;
}
bool flag_container::has_flag(const std::string& pName)
{
	return mFlags.find(pName) != mFlags.end();
}

void flag_container::load_script_interface(script_system & pScript)
{
	pScript.add_function("bool has_flag(const string &in)", asMETHOD(flag_container, has_flag), this);
	pScript.add_function("bool set_flag(const string &in)", asMETHOD(flag_container, set_flag), this);
	pScript.add_function("bool unset_flag(const string &in)", asMETHOD(flag_container, unset_flag), this);
}

void flag_container::clean()
{
	mFlags.clear();
}

void entity::set_dynamic_depth(bool pIs_dynamic)
{
	dynamic_depth = pIs_dynamic;
}

void entity::set_name(const std::string & pName)
{
	mName = pName;
}

const std::string& entity::get_name()
{
	return mName;
}

void entity::update_depth()
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
}

// #########
// sprite_entity
// #########

int sprite_entity::set_texture(std::string pName, texture_manager & pTexture_manager)
{
	auto texture = pTexture_manager.get_texture(pName);
	if (!texture)
	{
		util::error("Cannot find texture for entity");
		return 1;
	}
	mTexture = texture;
	return 0;
}

void sprite_entity::set_anchor(engine::anchor pAnchor)
{
	mSprite.set_anchor(pAnchor);
}

void sprite_entity::set_color(engine::color pColor)
{
	mSprite.set_color(pColor);
}

void sprite_entity::set_rotation(float pRotation)
{
	mSprite.set_rotation(pRotation);
}

engine::fvector sprite_entity::get_size() const
{
	return mSprite.get_size();
}

bool sprite_entity::is_playing() const
{
	return mSprite.is_playing();
}

sprite_entity::sprite_entity()
{
	set_dynamic_depth(true);
	mSprite.set_anchor(engine::anchor::bottom);
}

void sprite_entity::play_animation()
{
	mSprite.start();
}

void sprite_entity::stop_animation()
{
	mSprite.stop();
}

void sprite_entity::tick_animation()
{
	mSprite.tick();
}

bool sprite_entity::set_animation(const std::string& pName, bool pSwap)
{
	if (!mTexture)
		return false;

	auto animation = mTexture->get_animation(pName);
	if (!animation)
		return false;

	mSprite.set_animation(*animation, pSwap);
	return true;
}

int sprite_entity::draw(engine::renderer &_r)
{
	update_depth();

	mSprite.set_exact_position(get_exact_position());
	mSprite.draw(_r);
	return 0;
}

// #########
// character_entity
// #########

character_entity::character_entity()
{
	mCyclegroup = "default";
	mMove_speed = 3.f* defs::TILE_SIZE.x;
}

void
character_entity::set_cycle_group(std::string name)
{
	mCyclegroup = name;
	set_animation(mCyclegroup + ":" + mCycle, is_playing());
}

void
character_entity::set_cycle(const std::string& name)
{
	if (mCycle != name)
	{
		set_animation(mCyclegroup + ":" + name, is_playing());
		mCycle = name;
	}
}

void
character_entity::set_cycle(e_cycle type)
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
character_entity::set_speed(float f)
{
	mMove_speed = f;
}

float
character_entity::get_speed()
{
	return mMove_speed;
}

// #########
// collision_system
// #########

util::optional_pointer<collision_box> collision_system::wall_collision(const engine::frect& r)
{
	for (auto &i : mWalls)
		if (i.is_valid() && i.get_region().is_intersect(r))
			return &i;
	return{};
}

util::optional_pointer<door> collision_system::door_collision(const engine::fvector& pPosition)
{
	for (auto &i : mDoors)
		if (i.is_valid() && i.get_region().is_intersect(pPosition))
			return &i;
	return{};
}

util::optional_pointer<trigger> collision_system::trigger_collision(const engine::fvector& pPosition)
{
	for (auto &i : mTriggers)
		if (i.is_valid() && i.get_region().is_intersect(pPosition))
			return &i;
	return{};
}

util::optional_pointer<trigger> collision_system::button_collision(const engine::fvector& pPosition)
{
	for (auto &i : mButtons)
		if (i.is_valid() && i.get_region().is_intersect(pPosition))
			return &i;
	return{};
}

util::optional<engine::fvector> collision_system::get_door_entry(std::string pName)
{
	for (auto& i : mDoors)
	{
		if (i.name == pName)
		{
			engine::frect region = i.get_region();
			return region.get_offset() + (region.get_size()*0.5f) + i.offset;
		}
	}
	return{};
}

void collision_system::validate_all(flag_container& pFlags)
{
	for (auto &i : mWalls)    i.validate(pFlags);
	for (auto &i : mDoors)    i.validate(pFlags);
	for (auto &i : mTriggers) i.validate(pFlags);
	for (auto &i : mButtons)  i.validate(pFlags);
}

void collision_system::add_wall(engine::frect r)
{
	collision_box nw;
	nw.set_region(r);
	mWalls.push_back(nw);
}

void collision_system::add_trigger(trigger & t)
{
	mTriggers.push_back(t);
}

void collision_system::add_button(trigger & t)
{
	mButtons.push_back(t);
}

void collision_system::clean()
{
	mWalls.clear();
	mDoors.clear();
	mTriggers.clear();
	mButtons.clear();
}

int collision_system::load_collision_boxes(tinyxml2::XMLElement* pEle)
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
			set_cycle(character_entity::e_cycle::right);
		}
		else {
			mFacing_direction = direction::left;
			set_cycle(character_entity::e_cycle::left);
		}
	}
	else {
		if (pVec.y > 0)
		{
			mFacing_direction = direction::down;
			set_cycle(character_entity::e_cycle::down);
		}
		else {
			mFacing_direction = direction::up;
			set_cycle(character_entity::e_cycle::up);
		}
	}
}

player_character::player_character()
{
	mFacing_direction = direction::other;
}

void player_character::set_locked(bool pLocked)
{
	mLocked = pLocked;
}

bool player_character::is_locked()
{
	return mLocked;
}

void player_character::movement(controls& pControls, collision_system& pCollision_system, float pDelta)
{
	if (mLocked)
	{
		stop_animation();
		return;
	}

	engine::fvector move(0, 0);

	if (pControls.is_triggered(controls::control::left))
	{
		move.x -= 1;
		mFacing_direction = direction::left;
		set_cycle(character_entity::e_cycle::left);
	}

	if (pControls.is_triggered(controls::control::right))
	{
		move.x += 1;
		mFacing_direction = direction::right;
		set_cycle(character_entity::e_cycle::right);
	}

	if (pControls.is_triggered(controls::control::up))
	{
		move.y -= 1;
		mFacing_direction = direction::up;
		set_cycle(character_entity::e_cycle::up);
	}

	if (pControls.is_triggered(controls::control::down))
	{
		move.y += 1;
		mFacing_direction = direction::down;
		set_cycle(character_entity::e_cycle::down);
	}

	// Check collision if requested to move
	if (move != engine::fvector(0, 0))
	{
		move.normalize();
		move *= get_speed()*pDelta;

		const engine::fvector collision_size(26, 16);
		const engine::fvector collision_offset
			= get_position()
			- engine::fvector(collision_size.x / 2, collision_size.y);

		engine::frect collision(collision_offset, collision_size);

		collision.set_offset(collision_offset + engine::fvector(move.x, 0));
		if (pCollision_system.wall_collision(collision))
			move.x = 0;

		collision.set_offset(collision_offset + engine::fvector(0, move.y));
		if (pCollision_system.wall_collision(collision))
			move.y = 0;
	}

	// Move if possible
	if (move != engine::fvector(0, 0))
	{
		set_move_direction(move); // Make sure the player is in the direction he's moving
		set_position(get_position() + move);
		play_animation();
	}
	else
	{
		stop_animation();
	}
}

engine::fvector player_character::get_activation_point(float pDistance)
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
	mText_format = nullptr;
}

util::optional_pointer<entity> entity_manager::find_entity(const std::string& pName)
{
	for (auto &i : mEntities)
		if (i->get_name() == pName)
			return i.get();
	return{};
}

void entity_manager::clean()
{
	mEntities.clear();
}

void entity_manager::set_texture_manager(texture_manager& pTexture_manager)
{
	mTexture_manager = &pTexture_manager;
}

void entity_manager::set_text_format(const text_format_profile & pFormat)
{
	mText_format = &pFormat;
}

void entity_manager::register_entity_type(script_system & pScript)
{
	auto& engine = pScript.get_engine();

	engine.RegisterObjectType("entity", sizeof(entity_reference), asOBJ_VALUE | asGetTypeTraits<entity_reference>());

	// Constructors and deconstructors
	engine.RegisterObjectBehaviour("entity", asBEHAVE_CONSTRUCT, "void f()"
		, asFUNCTION(script_system::script_default_constructor<entity_reference>)
		, asCALL_CDECL_OBJLAST);
	engine.RegisterObjectBehaviour("entity", asBEHAVE_CONSTRUCT, "void f(const entity&in)"
		, asFUNCTIONPR(script_system::script_constructor<entity_reference>
			, (const entity_reference&, void*), void)
		, asCALL_CDECL_OBJLAST);
	engine.RegisterObjectBehaviour("entity", asBEHAVE_DESTRUCT, "void f()"
		, asFUNCTION(script_system::script_default_deconstructor<entity_reference>)
		, asCALL_CDECL_OBJLAST);

	// Assignments
	engine.RegisterObjectMethod("entity", "entity& opAssign(const entity&in)"
		, asMETHODPR(entity_reference, operator=, (const entity_reference&), entity_reference&)
		, asCALL_THISCALL);

	// is_valid
	engine.RegisterObjectMethod("entity", "bool is_valid() const"
		, asMETHOD(entity_reference, is_valid)
		, asCALL_THISCALL);

	engine.RegisterObjectMethod("entity", "void release()"
		, asMETHOD(entity_reference, reset)
		, asCALL_THISCALL);
}

bool entity_manager::check_entity(entity_reference & e)
{
	assert(mScript_system != nullptr);

	if (!e.is_valid())
	{
		util::error("(" + std::to_string(mScript_system->get_current_line()) + ") "
			+ "Entity object invalid.");
		return false;
	}
	return true;
}

entity_reference entity_manager::script_add_entity(const std::string & path)
{
	assert(get_renderer() != nullptr);
	assert(mTexture_manager != nullptr);

	if (mEntities.size() >= 256)
	{
		util::error("Reached upper limit of entities");
		return entity_reference();
	}

	auto ne = construct_entity<sprite_entity>();
	ne->set_texture(path, *mTexture_manager);
	ne->set_animation("default:default");
	get_renderer()->add_object(*ne);

	return *ne;
}

entity_reference entity_manager::script_add_entity_atlas(const std::string & path, const std::string& atlas)
{
	entity_reference e = script_add_entity(path);
	dynamic_cast<sprite_entity*>(e.get())->set_animation(atlas);
	return e;
}

entity_reference entity_manager::script_add_text()
{
	assert(get_renderer() != nullptr);
	assert(mTexture_manager != nullptr);
	assert(mText_format != nullptr);

	if (mEntities.size() >= 256)
	{
		util::error("Reached upper limit of entities");
		return entity_reference();
	}

	auto ne = construct_entity<text_entity>();
	ne->apply_format(*mText_format);
	get_renderer()->add_object(*ne);
	return *ne;
}

void entity_manager::script_set_text(entity_reference& e, const std::string & pText)
{
	if (!check_entity(e)) return;
	text_entity* c = dynamic_cast<text_entity*>(e.get());
	if (!c)
	{
		util::error("Entity is not text");
		return;
	}
	c->set_text(pText);
}

void entity_manager::script_remove_entity(entity_reference& e)
{
	if (!check_entity(e)) return;

	for (auto i = mEntities.begin(); i != mEntities.end(); i++)
	{
		if (i->get() == e.get())
		{
			mEntities.erase(i);
			return;
		}
	}

	util::error("Could not remove entity");
}

entity_reference entity_manager::script_add_character(const std::string & path)
{
	assert(get_renderer() != nullptr);
	assert(mTexture_manager != nullptr);

	if (mEntities.size() >= 256)
	{
		util::error("Reached upper limit of characters.");
		return entity_reference();
	}

	auto nc = construct_entity<character_entity>();
	nc->set_texture(path, *mTexture_manager);
	nc->set_cycle(character_entity::e_cycle::def);
	get_renderer()->add_object(*nc);

	return *nc;
}

void entity_manager::script_set_name(entity_reference& e, const std::string & pName)
{
	if (!check_entity(e)) return;
	e->set_name(pName);
}

void entity_manager::script_set_position(entity_reference& e, const engine::fvector & pos)
{
	if (!check_entity(e)) return;
	e->set_position(pos * 32);
}

engine::fvector entity_manager::script_get_position(entity_reference& e)
{
	if (!check_entity(e)) return engine::fvector(0, 0);
	return e->get_position()/32;
}

void entity_manager::script_set_direction(entity_reference& e, int dir)
{
	if (!check_entity(e)) return;
	character_entity* c = dynamic_cast<character_entity*>(e.get());
	if (!c)
	{
		util::error("Entity is not a character");
		return;
	}
	c->set_cycle(static_cast<character_entity::e_cycle>(dir));
}

void entity_manager::script_set_cycle(entity_reference& e, const std::string& name)
{
	if (!check_entity(e)) return;
	character_entity* c = dynamic_cast<character_entity*>(e.get());
	if (c == nullptr)
	{
		util::error("Entity is not a character");
		return;
	}
	c->set_cycle_group(name);
}

void entity_manager::script_set_depth(entity_reference& e, float pDepth)
{
	if (!check_entity(e)) return;
	e->set_dynamic_depth(false);
	e->set_depth(util::clamp(pDepth
		, defs::TILE_DEPTH_RANGE_MIN
		, defs::TILE_DEPTH_RANGE_MAX));
}

void entity_manager::script_set_depth_fixed(entity_reference& e, bool pFixed)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return;
	}
	se->set_dynamic_depth(!pFixed);
}

void entity_manager::script_start_animation(entity_reference& e)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return;
	}
	se->play_animation();
}

void entity_manager::script_stop_animation(entity_reference& e)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return;
	}
	se->stop_animation();
}

void entity_manager::script_set_animation(entity_reference& e, const std::string & name)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return;
	}
	se->set_animation(name);
}

void entity_manager::script_set_anchor(entity_reference& e, int pAnchor)
{
	if (!check_entity(e)) return;

	if (e->get_entity_type() == entity::entity_type::sprite)
	{
		auto se = dynamic_cast<sprite_entity*>(e.get());
		se->set_anchor(static_cast<engine::anchor>(pAnchor));
	}

	else if (e->get_entity_type() == entity::entity_type::text)
	{
		auto se = dynamic_cast<text_entity*>(e.get());
		se->set_anchor(static_cast<engine::anchor>(pAnchor));
	}
	else
	{
		util::error("Unsupported entity type");
	}
}

void entity_manager::script_set_rotation(entity_reference& e, float pRotation)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return;
	}
	se->set_rotation(std::fmod(std::abs(pRotation), 360.f));
}

void entity_manager::script_set_color(entity_reference& e, int r, int g, int b, int a)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return;
	}
	se->set_color(engine::color(r, g, b, a));
}

void entity_manager::script_set_visible(entity_reference & e, bool pIs_visible)
{
	if (!check_entity(e)) return;
	e->set_visible(pIs_visible);
}

void entity_manager::script_set_texture(entity_reference & e, const std::string & name)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return;
	}
	se->set_texture(name, *mTexture_manager);
	se->set_animation("default:default");
}

void entity_manager::script_add_child(entity_reference& e1, entity_reference& e2)
{
	if (!check_entity(e1)) return;
	if (!check_entity(e2)) return;

	e2->set_position(e2->get_position(*e1));
	e1->add_child(*e2);
}

void entity_manager::script_set_parent(entity_reference& e1, entity_reference& e2)
{
	if (!check_entity(e1)) return;
	if (!check_entity(e2)) return;

	e1->set_position(e1->get_position(*e2));
	e1->set_parent(*e2);
}

void entity_manager::script_detach_children(entity_reference& e)
{
	if (!check_entity(e)) return;

	auto children = e->get_children();
	for (auto& i : children)
	{
		i->set_position(i->get_position(*this));
		i->set_parent(*this);
	}
}

void entity_manager::script_detach_parent(entity_reference& e)
{
	if (!check_entity(e)) return;

	e->set_position(e->get_position(*this));
	e->set_parent(*this);
}

void entity_manager::script_make_gui(entity_reference & e, float pOffset)
{
	if (!check_entity(e)) return;

	// Gui elements essentually don't stick to anything
	// So we just detach everything.

	e->detach_children();
	e->detach_parent();

	e->set_dynamic_depth(false);
	e->set_depth(defs::GUI_DEPTH - (pOffset/1000));
}

void entity_manager::load_script_interface(script_system& pScript)
{
	mScript_system = &pScript;

	register_entity_type(pScript);

	pScript.add_function("entity add_entity(const string &in)",                      asMETHOD(entity_manager, script_add_entity), this);
	pScript.add_function("entity add_entity(const string &in, const string &in)",    asMETHOD(entity_manager, script_add_entity_atlas), this);
	pScript.add_function("entity add_text()",                                        asMETHOD(entity_manager, script_add_text), this);
	pScript.add_function("entity add_character(const string &in)",                   asMETHOD(entity_manager, script_add_character), this);
	pScript.add_function("void set_position(entity&in, const vec &in)",              asMETHOD(entity_manager, script_set_position), this);
	pScript.add_function("vec get_position(entity&in)",                              asMETHOD(entity_manager, script_get_position), this);
	pScript.add_function("void set_direction(entity&in, int)",                       asMETHOD(entity_manager, script_set_direction), this);
	pScript.add_function("void set_cycle(entity&in, const string &in)",              asMETHOD(entity_manager, script_set_cycle), this);
	pScript.add_function("void start_animation(entity&in)",                          asMETHOD(entity_manager, script_start_animation), this);
	pScript.add_function("void stop_animation(entity&in)",                           asMETHOD(entity_manager, script_stop_animation), this);
	pScript.add_function("void set_animation(entity&in, const string &in)",          asMETHOD(entity_manager, script_set_animation), this);
	pScript.add_function("entity find_entity(const string &in)",                     asMETHOD(entity_manager, find_entity), this);
	pScript.add_function("bool is_character(entity&in)",                             asMETHOD(entity_manager, is_character), this);
	pScript.add_function("void remove_entity(entity&in)",                            asMETHOD(entity_manager, script_remove_entity), this);
	pScript.add_function("void set_depth(entity&in, float)",                         asMETHOD(entity_manager, script_set_depth), this);
	pScript.add_function("void set_depth_fixed(entity&in, bool)",                    asMETHOD(entity_manager, script_set_depth_fixed), this);
	pScript.add_function("void set_name(entity&in, const string &in)",               asMETHOD(entity_manager, script_set_name), this);
	pScript.add_function("void _set_anchor(entity&in, int)",                         asMETHOD(entity_manager, script_set_anchor), this);
	pScript.add_function("void set_rotation(entity&in, float)",                      asMETHOD(entity_manager, script_set_rotation), this);
	pScript.add_function("void set_color(entity&in, int, int, int, int)",            asMETHOD(entity_manager, script_set_color), this);
	pScript.add_function("void set_visible(entity&in, bool)",                        asMETHOD(entity_manager, script_set_visible), this);
	pScript.add_function("void set_texture(entity&in, const string&in)",             asMETHOD(entity_manager, script_set_texture), this);
	pScript.add_function("void set_text(entity&in, const string &in)",               asMETHOD(entity_manager, script_set_text), this);

	pScript.add_function("void add_child(entity&in, entity&in)",                     asMETHOD(entity_manager, script_add_child), this);
	pScript.add_function("void set_parent(entity&in, entity&in)",                    asMETHOD(entity_manager, script_set_parent), this);
	pScript.add_function("void detach_children(entity&in)",                          asMETHOD(entity_manager, script_detach_children), this);
	pScript.add_function("void detach_parent(entity&in)",                            asMETHOD(entity_manager, script_detach_parent), this);

	pScript.add_function("void make_gui(entity&in, float)",                          asMETHOD(entity_manager, script_make_gui), this);
}

bool entity_manager::is_character(sprite_entity* pEntity)
{
	return dynamic_cast<character_entity*>(pEntity) != nullptr;
}

// #########
// scene
// #########

scene::scene()
{
	mTilemap_display.set_depth(defs::TILES_DEPTH);
	mWorld_node.add_child(mTilemap_display);
	mWorld_node.add_child(mEntity_manager);
	mWorld_node.add_child(mPlayer);
	mFocus_player = true;

	mPathfinding_system.set_collision_system(mCollision_system);
}

scene::~scene()
{
}

panning_node& scene::get_world_node()
{
	return mWorld_node;
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
	mColored_overlay.clean();
	mNarrative.end_narrative();
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

int scene::load_scene(std::string pPath)
{
	assert(mTexture_manager != nullptr);
	assert(mScript != nullptr);

	clean_scene();

	if (mLoader.load(pPath))
	{
		util::error("Unable to open scene");
		return 1;
	}

	mScene_path = pPath;
	mScene_name = mLoader.get_name();

	auto collision_boxes = mLoader.get_collisionboxes();
	if (collision_boxes)
		mCollision_system.load_collision_boxes(collision_boxes);

	mWorld_node.set_boundary_enable(mLoader.has_boundary());
	mWorld_node.set_boundary(mLoader.get_boundary());

	auto context = pScript_contexts[mScene_path];

	// Compile script if not already
	if (!context.is_valid())
	{
		context.set_script_system(*mScript);
		context.build_script(mLoader.get_script_path());
	}

	// Set context if still valid
	if (context.is_valid())
	{
		context.setup_triggers(mCollision_system);
		mScript->load_context(context);
	}

	auto tilemap_texture = mTexture_manager->get_texture(mLoader.get_tilemap_texture());
	if (!tilemap_texture)
	{
		util::error("Invalid tilemap texture");
		return 1;
	}
	mTilemap_display.set_texture(*tilemap_texture);

	mTilemap_loader.load_tilemap_xml(mLoader.get_tilemap());
	mTilemap_loader.update_display(mTilemap_display);

	// Pre-execute so the scene script can setup things before the render.
	mScript->tick();

	update_focus();

	return 0;
}

int
scene::reload_scene()
{
	if (mScene_path.empty())
	{
		util::error("No scene to reload");
		return 1;
	}

	clean_scene(true);

	// Recompile Scene
	pScript_contexts[mScene_path].clean();

	return load_scene(mScene_path);
}

const std::string& scene::get_path()
{
	return mScene_path;
}

const std::string& scene::get_name()
{
	return mScene_name;
}

void scene::load_script_interface(script_system& pScript)
{
	mEntity_manager.load_script_interface(pScript);
	mNarrative.load_script_interface(pScript);
	mBackground_music.load_script_interface(pScript);
	mColored_overlay.load_script_interface(pScript);
	mPathfinding_system.load_script_interface(pScript);

	pScript.add_function("void set_tile(const string &in, vec, int, int)", asMETHOD(scene, script_set_tile), this);
	pScript.add_function("void remove_tile(vec, int)", asMETHOD(scene, script_remove_tile), this);

	pScript.add_function("int _spawn_sound(const string&in, float, float)", asMETHOD(sound_manager, spawn_sound), &mSound_FX);
	pScript.add_function("void _stop_all()", asMETHOD(sound_manager, stop_all), &mSound_FX);

	pScript.add_function("entity get_player()", asMETHOD(scene, script_get_player), this);
	pScript.add_function("void _lockplayer(bool)", asMETHOD(player_character, set_locked), &mPlayer);

	pScript.add_function("void set_focus(vec)", asMETHOD(scene, script_set_focus), this);
	pScript.add_function("vec get_focus()", asMETHOD(scene, script_get_focus), this);
	pScript.add_function("void focus_player(bool)", asMETHOD(scene, focus_player), this);

	pScript.add_function("vec get_boundary_position()", asMETHOD(scene, script_get_boundary_position), this);
	pScript.add_function("vec get_boundary_size()", asMETHOD(scene, script_get_boundary_size), this);
	pScript.add_function("void get_boundary_position(vec)", asMETHOD(scene, script_set_boundary_position), this);
	pScript.add_function("void get_boundary_size(vec)", asMETHOD(scene, script_set_boundary_size), this);
	pScript.add_function("void set_boundary_enable(bool)", asMETHOD(panning_node, set_boundary_enable), this);

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

	mPlayer.set_texture(att_texture, *mTexture_manager);
	mPlayer.set_cycle(character_entity::e_cycle::def);

	auto ele_sounds = ele_root->FirstChildElement("sounds");
	if (ele_sounds)
	{
		if (!ele_sounds->Attribute("path"))
		{
			util::error("Please specify path of sounds");
			return;
		}
		mSound_FX.load_from_directory(ele_sounds->Attribute("path"));
	}

	if (auto ele_narrative = ele_root->FirstChildElement("narrative"))
	{
		assert(mTexture_manager != nullptr);
		mNarrative.load_narrative_xml(ele_narrative, *mTexture_manager);
	}
}

player_character& scene::get_player()
{
	return mPlayer;
}

void scene::tick(controls &pControls)
{
	assert(get_renderer() != nullptr);
	mPlayer.movement(pControls, mCollision_system, get_renderer()->get_delta());
	update_focus();
	update_collision_interaction(pControls);
}

void scene::focus_player(bool pFocus)
{
	mFocus_player = pFocus;
}

inline void rpg::scene::set_text_format(const text_format_profile & pFormat)
{
	mEntity_manager.set_text_format(pFormat);
	mNarrative.set_text_format(pFormat);
}

void scene::script_set_focus(engine::fvector pPosition)
{
	mFocus_player = false;
	mWorld_node.set_focus(pPosition * 32);
}

engine::fvector scene::script_get_focus()
{
	return mWorld_node.get_focus() / 32;
}

entity_reference scene::script_get_player()
{
	return mPlayer;
}

engine::fvector scene::script_get_boundary_position()
{
	return mWorld_node.get_boundary().get_offset();
}

engine::fvector scene::script_get_boundary_size()
{
	return mWorld_node.get_boundary().get_size();
}

void scene::script_set_boundary_size(engine::fvector pSize)
{
	mWorld_node.set_boundary(mWorld_node.get_boundary().set_size(pSize));
}

void scene::script_set_boundary_position(engine::fvector pPosition)
{
	mWorld_node.set_boundary(mWorld_node.get_boundary().set_offset(pPosition));
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
	mWorld_node.set_viewport(pR.get_size());
	pR.add_object(mTilemap_display);
	pR.add_object(mNarrative);
	pR.add_object(mPlayer);
	mColored_overlay.set_renderer(pR);
	mEntity_manager.set_renderer(pR);
}

void scene::update_focus()
{
	if (mFocus_player)
		mWorld_node.set_focus(mPlayer.get_position(mWorld_node));
}

void scene::update_collision_interaction(controls & pControls)
{
	auto player_position = mPlayer.get_position();

	{
		auto trigger = mCollision_system.trigger_collision(player_position);
		if (trigger)
		{
			if (trigger->get_function().call())
				trigger->get_function().set_arg(0, &player_position);
		}
	}

	{
		auto door = mCollision_system.door_collision(player_position);
		if (door)
		{
			std::string destination = door->destination;
			load_scene(door->scene_path);
			auto new_position = mCollision_system.get_door_entry(destination);
			if (!new_position)
				util::error("Destination door '" + destination + "' does not exist");
			else
				mPlayer.set_position(*new_position);
		}
	}

	if (pControls.is_triggered(controls::control::activate))
	{
		auto button = mCollision_system.button_collision(player_position);
		if (button)
		{
			if (button->get_function().call())
				button->get_function().set_arg(0, &player_position);
		}
	}
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
	mControls.fill(false);
}

void controls::update(engine::renderer & pR)
{
	using key_type = engine::renderer::key_type;
	using control = rpg::controls::control;

	reset();

	if (pR.is_key_pressed(key_type::Z) ||
		pR.is_key_pressed(key_type::Return))
		trigger(control::activate);

	if (pR.is_key_down(key_type::Left))
		trigger(control::left);

	if (pR.is_key_down(key_type::Right))
		trigger(control::right);

	if (pR.is_key_down(key_type::Up))
		trigger(control::up);

	if (pR.is_key_down(key_type::Down))
		trigger(control::down);

	if (pR.is_key_pressed(key_type::Up))
		trigger(control::select_up);

	if (pR.is_key_pressed(key_type::Down))
		trigger(control::select_down);

	if (pR.is_key_pressed(key_type::Left))
		trigger(control::select_previous);

	if (pR.is_key_pressed(key_type::Right))
		trigger(control::select_next);

	if (pR.is_key_down(key_type::LControl))
	{
		if (pR.is_key_pressed(key_type::R))
			trigger(control::reset);
		if (pR.is_key_pressed(key_type::Num1))
			trigger(control::editor_1);
		if (pR.is_key_pressed(key_type::Num2))
			trigger(control::editor_2);
	}
}

// #########
// angelscript
// #########

void script_system::message_callback(const asSMessageInfo * msg)
{
	log_entry_type type = log_entry_type::error;
	if (msg->type == asEMsgType::asMSGTYPE_INFORMATION)
		type = log_entry_type::info;
	else if(msg->type == asEMsgType::asMSGTYPE_WARNING)
		type = log_entry_type::warning;

	log_print(msg->section,msg->row, msg->col, type, msg->message);
}

std::string
script_context::get_metadata_type(const std::string & pMetadata)
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

void script_system::script_create_thread(AS::asIScriptFunction * func, AS::CScriptDictionary * arg)
{
	if (func == 0)
	{
		util::error("Invalid function");
		return;
	}

	// Create a new context for the co-routine
	asIScriptContext *ctx = mCtxmgr.AddContext(mEngine, func);

	// Pass the argument to the context
	ctx->SetArgObject(0, arg);
}

void script_system::script_create_thread_noargs(AS::asIScriptFunction * func)
{
	if (func == 0)
	{
		util::error("Invalid function");
		return;
	}

	// Create a new context for the co-routine
	mCtxmgr.AddContext(mEngine, func);
}

void script_system::load_script_interface()
{
	add_function("int rand()", asFUNCTION(std::rand));
	add_function("void _timer_start(float)", asMETHOD(engine::timer, start), &mTimer);
	add_function("bool _timer_reached()", asMETHOD(engine::timer, is_reached), &mTimer);

	add_function("void create_thread(coroutine @+)", asMETHOD(script_system, script_create_thread_noargs), this);
	add_function("void create_thread(coroutine @+, dictionary @+)", asMETHOD(script_system, script_create_thread), this);

	add_function("void dprint(const string &in)", asMETHOD(script_system, debug_print), this);
	add_function("void eprint(const string &in)", asMETHOD(script_system, error_print), this);
	add_function("void abort()", asMETHOD(script_system, script_abort), this);
}


void script_system::log_print(const std::string& pFile, int pLine, int pCol, log_entry_type pType, const std::string& pMessage)
{
	if (!mLog_file.is_open()) // Open the log file when needed
		mLog_file.open("./data/log.txt");

	std::string type;
	switch (pType)
	{
	case log_entry_type::error:   type = "ERROR";   break;
	case log_entry_type::info:    type = "INFO";    break;
	case log_entry_type::warning: type = "WARNING"; break;
	case log_entry_type::debug:   type = "DEBUG";   break;
	}

	std::string message = pFile;
	message += " ( ";
	message += std::to_string(pLine);
	message += ", ";
	message += std::to_string(pCol);
	message += " ) : ";
	message += type;
	message += " : ";
	message += pMessage;
	message += "\n";

	if (mLog_file.is_open())
		mLog_file << message;

	std::cout << message;
}

void
script_system::debug_print(std::string &pMessage)
{
	if (!is_executing())
	{
		log_print("Unknown", 0, 0, log_entry_type::debug, pMessage);
		return;
	}

	assert(mCtxmgr.GetCurrentContext() != nullptr);
	assert(mCtxmgr.GetCurrentContext()->GetFunction() != nullptr);

	std::string name = mCtxmgr.GetCurrentContext()->GetFunction()->GetName();
	log_print(name, get_current_line(), 0, log_entry_type::debug, pMessage);
}

void script_system::error_print(std::string & pMessage)
{
	if (!is_executing())
	{
		log_print("Unknown", 0, 0, log_entry_type::error, pMessage);
		return;
	}

	assert(mCtxmgr.GetCurrentContext() != nullptr);
	assert(mCtxmgr.GetCurrentContext()->GetFunction() != nullptr);

	std::string name = mCtxmgr.GetCurrentContext()->GetFunction()->GetName();
	log_print(name, get_current_line(), 0, log_entry_type::error, pMessage);
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
	RegisterScriptDictionary(mEngine);
	mCtxmgr.RegisterCoRoutineSupport(mEngine);

	register_vector_type();

	load_script_interface();

	mExecuting = false;
}

script_system::~script_system()
{
	mCtxmgr.AbortAll();
	mCtxmgr.~CContextMgr(); // Destroy context manager before releasing engine
	mEngine->ShutDownAndRelease();
}

void script_system::load_context(script_context & pContext)
{
	mCtxmgr.AbortAll();
	mContext = &pContext;
	start_all_with_tag("start");
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

void script_system::about_all()
{
	if (is_executing())
		mCtxmgr.PreAbout();
	else
		mCtxmgr.AbortAll();
}

void script_system::start_all_with_tag(const std::string & pTag)
{
	size_t func_count = mContext->mScene_module->GetFunctionCount();
	for (size_t i = 0; i < func_count; i++)
	{
		auto func = mContext->mScene_module->GetFunctionByIndex(i);
		std::string metadata = parsers::remove_trailing_whitespace(mContext->mBuilder.GetMetadataStringForFunc(func));
		if (metadata == pTag)
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

int script_system::get_current_line()
{
	if (mCtxmgr.GetCurrentContext())
	{
		return mCtxmgr.GetCurrentContext()->GetLineNumber();
	}
	return 0;
}

AS::asIScriptEngine& rpg::script_system::get_engine()
{
	assert(mEngine != nullptr);
	return *mEngine;
}

bool script_system::is_executing()
{
	return mExecuting;
}

// #########
// script_context
// #########

script_context::script_context() :
	mScene_module(nullptr)
{}

void script_context::set_script_system(script_system & pScript)
{
	mScript = &pScript;
}

bool script_context::build_script(const std::string & pPath)
{
	mBuilder.StartNewModule(&mScript->get_engine(), pPath.c_str());
	mBuilder.AddSectionFromMemory("scene_commands", "#include 'data/internal/scene.as'");
	mBuilder.AddSectionFromFile(pPath.c_str());
	if (mBuilder.BuildModule())
	{
		util::error("Failed to load scene script");
		return false;
	}
	mScene_module = mBuilder.GetModule();
	return true;
}

bool script_context::is_valid()
{
	return mScene_module.has_value();
}

void script_context::clean()
{
	if (mScene_module)
	{
		mScene_module->Discard();
		mScene_module = nullptr;
	}
}

void
script_context::setup_triggers(collision_system& pCollision_system)
{
	size_t func_count = mScene_module->GetFunctionCount();
	for (size_t i = 0; i < func_count; i++)
	{
		auto func = mScene_module->GetFunctionByIndex(i);
		std::string metadata = parsers::remove_trailing_whitespace(mBuilder.GetMetadataStringForFunc(func));
		std::string type = get_metadata_type(metadata);

		if (type == "trigger" ||
			type == "button")
		{
			trigger nt;
			script_function& sfunc = nt.get_function();
			sfunc.set_context_manager(&mScript->mCtxmgr);
			sfunc.set_engine(mScript->mEngine);
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

void game::save_game()
{
	const std::string path = get_slot_path(mSlot);
	save_system file;
	file.new_save();
	file.save_flags(mFlags);
	file.save_scene(mScene);
	file.save_player(mScene.get_player());
	file.save(path);
}

void game::open_game()
{
	const std::string path = get_slot_path(mSlot);
	save_system file;
	if (!file.open_save(path))
	{
		util::error("Invalid slot");
		return;
	}
	mFlags.clean();
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

void game::set_slot(size_t pSlot)
{
	mSlot = pSlot;
}

size_t game::get_slot()
{
	return mSlot;
}

void game::script_load_scene(const std::string & pPath)
{
	mRequest_load = true;
	mNew_scene_path = pPath;
}

void
game::load_script_interface()
{
	mScript.add_function("float get_delta()", asMETHOD(game, get_delta), this);

	mScript.add_function("bool _is_triggered(int)", asMETHOD(controls, is_triggered), &mControls);

	mScript.add_function("void save_game()", asMETHOD(game, save_game), this);
	mScript.add_function("void open_game()", asMETHOD(game, open_game), this);
	mScript.add_function("uint get_slot()", asMETHOD(game, get_slot), this);
	mScript.add_function("void set_slot(uint)", asMETHOD(game, set_slot), this);
	mScript.add_function("bool is_slot_used(uint)", asMETHOD(game, is_slot_used), this);
	mScript.add_function("void load_scene(const string &in)", asMETHOD(game, script_load_scene), this);

	mFlags.load_script_interface(mScript);
	mScene.load_script_interface(mScript);
}

float game::get_delta()
{
	return get_renderer()->get_delta();
}

int
game::load_game_xml(std::string pPath)
{
	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
	{
		util::error("Could not load game file at '" + pPath + "'");
		return 1;
	}
	auto ele_root = doc.RootElement();

	auto ele_scene = ele_root->FirstChildElement("scene");
	if (!ele_scene)
	{
		util::error("Please specify the scene to start with");
		return 1;
	}
	std::string scene_path = util::safe_string(ele_scene->Attribute("path"));

	auto ele_textures = ele_root->FirstChildElement("textures");
	if (!ele_textures ||
		!ele_textures->Attribute("path"))
	{
		util::error("Please specify the texture directory's path");
		return 1;
	}
	mTexture_manager.load_from_directory(ele_textures->Attribute("path"));

	if (auto ele_narrative = ele_root->FirstChildElement("narrative"))
	{
		mDefault_format.load_settings(ele_narrative);
	}

	mScene.set_text_format(mDefault_format);
	mScene.set_texture_manager(mTexture_manager);
	mScene.load_game_xml(ele_root);
	mScene.load_scene(scene_path);
	return 0;
}

void
game::tick()
{
	mControls.update(*get_renderer());
	if (mControls.is_triggered(controls::control::reset))
	{
		mEditor_manager.close_editor();
		std::cout << "Reloading scene...\n";
		mScene.reload_scene();
		std::cout << "Done\n";
	}

	if (mControls.is_triggered(controls::control::editor_1))
	{
		mEditor_manager.close_editor();
		mEditor_manager.open_tilemap_editor(mScene.get_path());
		mScene.clean_scene(true);
	}

	if (mControls.is_triggered(controls::control::editor_2))
	{
		mEditor_manager.close_editor();
		mEditor_manager.open_collisionbox_editor(mScene.get_path());
		mScene.clean_scene(true);
	}
	if (mEditor_manager.is_editor_open())
		return;

	mScene.tick(mControls);

	mScript.tick();
	if (mRequest_load)
	{
		mRequest_load = false;
		mScene.load_scene(mNew_scene_path);
	}

	mEditor_manager.update_camera_position(mScene.get_world_node().get_exact_position());
}

void
game::refresh_renderer(engine::renderer & pR)
{
	mScene.set_renderer(pR);
	pR.add_object(mEditor_manager);
	pR.set_icon("data/icon.png");
}

// ##########
// panning_node
// ##########

panning_node::panning_node()
{
	mBoundary_enabled = true;
}

void
panning_node::set_boundary(engine::frect pBoundary)
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
	mFocus = pFocus;

	if (!mBoundary_enabled)
	{
		set_position(-(pFocus - (mViewport * 0.5f)));
		return;
	}

	engine::fvector offset = pFocus - (mViewport * 0.5f) - mBoundary.get_offset();
	if (mBoundary.w < mViewport.x)
		offset.x = (mBoundary.w * 0.5f) - (mViewport.x * 0.5f);
	else
		offset.x = util::clamp(offset.x, 0.f, mBoundary.w - mViewport.x);

	if (mBoundary.h < mViewport.y)
		offset.y = (mBoundary.h * 0.5f) - (mViewport.y * 0.5f);
	else
		offset.y = util::clamp(offset.y, 0.f, mBoundary.h - mViewport.y);

	set_position(-(offset + mBoundary.get_offset()));
}

engine::frect panning_node::get_boundary()
{
	return mBoundary;
}

engine::fvector panning_node::get_focus()
{
	return mFocus;
}

void panning_node::set_boundary_enable(bool pEnable)
{
	mBoundary_enabled = pEnable;
}

// ##########
// narrative_dialog
// ##########

int
narrative_dialog::load_box(tinyxml2::XMLElement* pEle, texture_manager& pTexture_manager)
{
	auto ele_box = pEle->FirstChildElement("box");

	std::string att_box_tex = ele_box->Attribute("texture");
	std::string att_box_atlas = ele_box->Attribute("atlas");

	mBox.set_texture(att_box_tex, pTexture_manager);
	mBox.set_animation(att_box_atlas);

	mBox.set_anchor(engine::anchor::topleft);
	set_box_position(position::bottom);

	mSelection.set_position(mBox.get_size() - engine::fvector(20, 10));
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

entity_reference narrative_dialog::script_get_narrative_box()
{
	return mBox;
}

entity_reference narrative_dialog::script_get_narrative_text()
{
	return mText;
}

narrative_dialog::narrative_dialog()
{
	hide_box();

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
void narrative_dialog::show_box()
{
	mText.set_visible(true);
	mBox.set_visible(true);
}

void narrative_dialog::hide_box()
{
	mText.set_visible(false);
	mBox.set_visible(false);
}

void narrative_dialog::end_narrative()
{
	hide_box();
	mSelection.set_visible(false);
	mExpression.set_visible(false);
	reset_positions();
	mText.set_interval(defs::DEFAULT_DIALOG_SPEED);
}

bool narrative_dialog::is_box_open()
{
	return mBox.is_visible() || mText.is_visible();
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

int narrative_dialog::load_narrative_xml(tinyxml2::XMLElement* pEle, texture_manager& pTexture_manager)
{
	load_box(pEle, pTexture_manager);

	if (auto ele_expressions = pEle->FirstChildElement("expressions"))
	{
		mExpression_manager.load_expressions_xml(ele_expressions, pTexture_manager);
	}
	return 0;
}

void narrative_dialog::load_script_interface(script_system & pScript)
{
	pScript.add_function("void _say(const string &in, bool)", asMETHOD(dialog_text_entity, reveal), &mText);
	pScript.add_function("bool _is_revealing()", asMETHOD(dialog_text_entity, is_revealing), &mText);
	pScript.add_function("void _skip_reveal()", asMETHOD(dialog_text_entity, skip_reveal), &mText);
	pScript.add_function("void _set_interval(float)", asMETHOD(dialog_text_entity, set_interval), &mText);
	pScript.add_function("bool _has_displayed_new_character()", asMETHOD(dialog_text_entity, has_revealed_character), &mText);

	pScript.add_function("void _showbox()", asMETHOD(narrative_dialog, show_box), this);
	pScript.add_function("void _hidebox()", asMETHOD(narrative_dialog, hide_box), this);
	pScript.add_function("void _end_narrative()", asMETHOD(narrative_dialog, end_narrative), this);
	pScript.add_function("void _show_selection()", asMETHOD(narrative_dialog, show_selection), this);
	pScript.add_function("void _hide_selection()", asMETHOD(narrative_dialog, hide_selection), this);
	pScript.add_function("void _set_box_position(int)", asMETHOD(narrative_dialog, set_box_position), this);
	pScript.add_function("void _set_selection(const string &in)", asMETHOD(narrative_dialog, set_selection), this);
	pScript.add_function("bool _is_box_open()", asMETHOD(narrative_dialog, is_box_open), this);

	pScript.add_function("void _set_expression(const string &in)", asMETHOD(narrative_dialog, set_expression), this);
	pScript.add_function("void _start_expression_animation()", asMETHOD(engine::animation_node, start), &mExpression);
	pScript.add_function("void _stop_expression_animation()", asMETHOD(engine::animation_node, stop), &mExpression);

	pScript.add_function("entity _get_narrative_box()", asMETHOD(narrative_dialog, script_get_narrative_box), this);
	pScript.add_function("entity _get_narrative_text()", asMETHOD(narrative_dialog, script_get_narrative_text), this);

}

int narrative_dialog::draw(engine::renderer& pR)
{
	return 0;
}

void narrative_dialog::set_text_format(const text_format_profile & pFormat)
{
	mText.apply_format(pFormat);
	pFormat.apply_to(mSelection);
}

void narrative_dialog::refresh_renderer(engine::renderer& r)
{
	r.add_object(mBox);
	r.add_object(mText);
	r.add_object(mSelection);
	r.add_object(mExpression);
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
	//func->Release();
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
	if(index < func->GetParamCount())
		func_ctx->SetArgObject(index, ptr);
}

bool
script_function::call()
{
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
tilemap_loader::find_tile(engine::fvector pos, int layer)
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
	for (auto i = pMap.begin(); i != pMap.end(); i++)
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

int
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

int
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

int
tilemap_loader::load_tilemap_xml(std::string pPath)
{
	using namespace tinyxml2;
	XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
	{
		util::error("Error loading tilemap file");
		return 1;
	}
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
	for (auto i = mMap[pLayer].begin(); i != mMap[pLayer].end(); i++)
	{
		if (i->second.position == pPosition)
		{
			mMap[pLayer].erase(i);
			break;
		}
	}
}

std::string tilemap_loader::find_tile_name(engine::fvector pPosition, int pLayer)
{
	auto f = find_tile_at(pPosition, pLayer);
	if (!f)
		return std::string();
	return f->atlas;
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
	mRegion = engine::scale(rect, defs::TILE_SIZE);
}

engine::frect collision_box::get_region()
{
	return mRegion;
}

void collision_box::set_region(engine::frect pRegion)
{
	mRegion = pRegion;
}

script_function& trigger::get_function()
{
	return mFunc;
}

void trigger::parse_function_metadata(const std::string & pMetadata)
{
	auto rect = parsers::parse_attribute_rect<float>(pMetadata);
	mRegion = engine::scale(rect, 32);
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
		vb.set_position(get_exact_position().floor());
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

	if (pAnimation->get_frame_count() > 0) // Start timer if this is an animation
		mTimer.start(pAnimation->get_interval()*0.001f);
}

void tilemap_display::tile::update_animation()
{
	if (!mAnimation) return;
	if (!mAnimation->get_frame_count()) return;
	if (mTimer.is_reached())
	{
		++mFrame;
		mTimer.start(mAnimation->get_interval(mFrame)*0.001f);
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

util::optional_pointer<const engine::animation> expression_manager::find_animation(const std::string & mName)
{
	auto find = mAnimations.find(mName);
	if (find != mAnimations.end())
	{
		return find->second;
	}
	return{};
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
		if (!texture)
		{
			util::error("Failed to load expression '" + std::string(att_texture) + "'");
			continue;
		}

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
// colored_overlay
// ##########

colored_overlay::colored_overlay()
{
	clean();
	mOverlay.set_depth(-10000);
}

void colored_overlay::load_script_interface(script_system & pScript)
{
	pScript.add_function("void set_overlay_color(int, int, int)", asMETHOD(colored_overlay, script_set_overlay_color), this);
	pScript.add_function("void set_overlay_opacity(int)", asMETHOD(colored_overlay, script_set_overlay_opacity), this);
}

void colored_overlay::clean()
{
	mOverlay.set_color({ 0, 0, 0, 0 });
}

void colored_overlay::refresh_renderer(engine::renderer& pR)
{
	pR.add_object(mOverlay);
	mOverlay.set_size(pR.get_size());
}

void colored_overlay::script_set_overlay_color(int r, int g, int b)
{
	auto color = mOverlay.get_color();
	mOverlay.set_color(engine::color(
		util::clamp(r, 0, 255)
		, util::clamp(g, 0, 255)
		, util::clamp(b, 0, 255)
		, color.a ));
}

void colored_overlay::script_set_overlay_opacity(int a)
{
	auto color = mOverlay.get_color();
	color.a = util::clamp(a, 0, 255);
	mOverlay.set_color(color);
}

// ##########
// pathfinding_system
// ##########

pathfinding_system::pathfinding_system()
{
	mPathfinder.set_collision_callback(
		[&](engine::fvector& pos) ->bool
	{
		return mCollision_system->wall_collision({ (pos * 32), { 31, 31 } }).has_value();
	});
}

void pathfinding_system::set_collision_system(collision_system & pCollision_system)
{
	mCollision_system = &pCollision_system;
}

void pathfinding_system::load_script_interface(script_system & pScript)
{
	pScript.add_function("bool find_path(array<vec>&inout, vec, vec)", asMETHOD(pathfinding_system, script_find_path), this);
	pScript.add_function("bool find_path_partial(array<vec>&inout, vec, vec, int)", asMETHOD(pathfinding_system, script_find_path_partial), this);
}

bool pathfinding_system::script_find_path(AS::CScriptArray& pScript_path, engine::fvector pStart, engine::fvector pDestination)
{
	mPathfinder.set_path_limit(1000);

	if (mPathfinder.start(pStart, pDestination))
	{
		auto path = mPathfinder.construct_path();
		for (auto& i : path)
			pScript_path.InsertLast(&i);
		return true;
	}
	return false;
}

bool pathfinding_system::script_find_path_partial(AS::CScriptArray& pScript_path, engine::fvector pStart, engine::fvector pDestination, int pCount)
{
	mPathfinder.set_path_limit(pCount);

	bool retval = mPathfinder.start(pStart, pDestination);

	auto path = mPathfinder.construct_path();
	for (auto& i : path)
		pScript_path.InsertLast(&i);
	return retval;
}

// #########
// text_format_profile
// #########

int text_format_profile::load_settings(tinyxml2::XMLElement * pEle)
{
	auto ele_font = pEle->FirstChildElement("font");

	std::string att_path = util::safe_string(ele_font->Attribute("path"));
	if (att_path.empty())
	{
		util::error("Please specify font path");
		return 1;
	}
	mFont.load(att_path);

	auto att_size = ele_font->IntAttribute("size");
	if (att_size > 0)
		mCharacter_size = att_size;
	else
		mCharacter_size = 9;

	auto att_scale = ele_font->FloatAttribute("scale");
	if (att_scale > 0)
		mScale = att_scale;
	else
		mScale = 1;

	return 0;
}

int text_format_profile::get_character_size() const
{
	return mCharacter_size;
}

float text_format_profile::get_scale() const
{
	return mScale;
}

const engine::font& text_format_profile::get_font() const
{
	return mFont;
}

void text_format_profile::apply_to(engine::text_node& pText) const
{
	pText.set_font(mFont);
	pText.set_character_size(mCharacter_size);
	pText.set_scale(mScale);
	//pText.set_color(mColor);
}

// #########
// dialog_text_entity
// #########

dialog_text_entity::dialog_text_entity()
{
	set_interval(defs::DEFAULT_DIALOG_SPEED);
}

void dialog_text_entity::clear()
{
	mRevealing = false;
	mText.set_text("");
}


int dialog_text_entity::draw(engine::renderer & pR)
{
	mNew_character = false;
	if (mRevealing)
		do_reveal();

	mText.draw(pR);
	return 0;
}

bool dialog_text_entity::is_revealing()
{
	return mRevealing;
}

void dialog_text_entity::reveal(const std::string & pText, bool pAppend)
{
	if (pText.empty())
		return;

	mTimer.start();
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

void dialog_text_entity::skip_reveal()
{
	if (mRevealing)
		mCount = mFull_text.length();
}

void dialog_text_entity::set_interval(float pMilliseconds)
{
	mTimer.set_interval(pMilliseconds*0.001f);
}

bool dialog_text_entity::has_revealed_character()
{
	return mNew_character;
}

void dialog_text_entity::do_reveal()
{
	size_t iterations = mTimer.get_count();
	if (iterations > 0)
	{
		mCount += iterations;
		mCount = util::clamp<size_t>(mCount, 0, mFull_text.size());

		std::string display(mFull_text.begin(), mFull_text.begin() + mCount);
		mText.set_text(display);

		if (mCount == mFull_text.size())
			mRevealing = false;

		mNew_character = true;

		mTimer.start();
	}
}

text_entity::text_entity()
{
	set_dynamic_depth(false);
	add_child(mText);
}

void text_entity::apply_format(const text_format_profile & pFormat)
{
	pFormat.apply_to(mText);
}

void text_entity::set_text(const std::string & pText)
{
	mText.set_text(pText);
}

void text_entity::set_anchor(engine::anchor pAnchor)
{
	mText.set_anchor(pAnchor);
}

int text_entity::draw(engine::renderer & pR)
{
	update_depth();
	mText.set_exact_position(get_exact_position());
	mText.draw(pR);
	return 0;
}
