#include "rpg.hpp"

#include <angelscript/add_on/scriptstdstring/scriptstdstring.h>
#include <angelscript/add_on/scriptmath/scriptmath.h>
#include <angelscript/add_on/scriptarray/scriptarray.h>

#include "parsers.hpp"
#include "tinyxml2\xmlshortcuts.hpp"

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
entity::load_entity_xml(std::string path, texture_manager & tm)
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
	mNode.set_anchor(engine::anchor::bottom);
	add_child(mNode);
}

void
entity::play_withtype(e_type pType)
{
	if (mAnimation
		&& mAnimation->type == pType)
		mNode.start();
}

void
entity::stop_withtype(e_type pType)
{
	if (mAnimation
		&& mAnimation->type == pType)
		mNode.stop();
}

void
entity::tick_withtype(e_type pType)
{
	if (mAnimation
		&& mAnimation->type == pType)
		mNode.tick();
}

bool
entity::set_animation(std::string pName)
{
	for (auto &i : animations)
	{
		if (i.get_name() == pName)
		{
			mAnimation = &i;
			mNode.set_animation(i.anim, true);
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
		float ndepth = defs::TILE_DEPTH_RANGE_MAX
			- util::clamp(get_position().y / 32
				, defs::TILE_DEPTH_RANGE_MIN
				, defs::TILE_DEPTH_RANGE_MAX);
		if (ndepth != get_depth())
			set_depth(ndepth);
	}
	mNode.draw(_r);
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

		load_single_animation(ele, anim.anim, tm);
		ele = ele->NextSiblingElement();
	}

	return 0;
}

util::error
entity::load_single_animation(tinyxml2::XMLElement* ele, engine::animation &anim, texture_manager &tm)
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
	anim.set_texture(*t);

	engine::animation::e_loop loop_type = engine::animation::e_loop::none;
	if (att_loop)             loop_type = engine::animation::e_loop::linear;
	if (att_pingpong)         loop_type = engine::animation::e_loop::pingpong;
	anim.set_loop(loop_type);

	anim.add_interval(0, att_interval);

	engine::frame_t frame_count = (att_frames <= 0 ? 1 : att_frames);// Default one frame
	anim.set_frame_count(frame_count);

	anim.set_default_frame(att_default);

	{
		auto atlas = t->get_entry(att_atlas);
		if (atlas.w == 0)
			return "Atlas '" + std::string(att_atlas) + "' does not exist";
		anim.set_frame_rect(atlas);
	}

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
	set_cycle(cycle);
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
// collision_system
// #########

collision_box*
collision_system::wall_collision(const engine::frect & r)
{
	for (auto &i : mWalls)
		if (i.is_valid() && i.is_intersect(r))
			return &i;
	return nullptr;
}

door*
collision_system::door_collision(const engine::fvector & r)
{
	for (auto &i : mDoors)
		if (i.is_valid() && i.is_intersect(r))
			return &i;
	return nullptr;
}

trigger*
collision_system::trigger_collision(const engine::fvector & pos)
{
	for (auto &i : mTriggers)
		if (i.is_valid() && i.is_intersect(pos))
			return &i;
	return nullptr;
}

trigger*
collision_system::button_collision(const engine::fvector & pos)
{
	for (auto &i : mButtons)
		if (i.is_valid() && i.is_intersect(pos))
			return &i;
	return nullptr;
}

engine::fvector
collision_system::get_door_entry(std::string name)
{
	for (auto& i : mDoors)
	{
		if (i.name == name)
		{
			return i.get_offset() + (i.get_size()*0.5f);
		}
	}
	return{ -1, -1};
}

void 
collision_system::validate_all(flag_container& flags)
{
	for (auto &i : mWalls)    i.validate(flags);
	for (auto &i : mDoors)    i.validate(flags);
	for (auto &i : mTriggers) i.validate(flags);
	for (auto &i : mButtons)  i.validate(flags);
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
collision_system::clear()
{
	mWalls.clear();
	mDoors.clear();
	mTriggers.clear();
	mButtons.clear();
}

util::error
collision_system::load_collision_boxes(tinyxml2::XMLElement* e, flag_container& flags)
{
	assert(e != nullptr);

	auto ele_item = e->FirstChildElement();
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
			nd.name = util::safe_string(ele_item->Attribute("name"));
			nd.destination = util::safe_string(ele_item->Attribute("dest"));
			nd.scene_path = util::safe_string(ele_item->Attribute("scene"));
			mDoors.push_back(nd);
		}

		ele_item = ele_item->NextSiblingElement();
	}
	return 0;
}

// #########
// player_character
// #########

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

void
player_character::movement(controls& pControls, collision_system& pCollision_system, float pDelta)
{
	if (mLocked)
	{
		stop_withtype(entity::e_type::movement);
		return;
	}

	engine::fvector pos = get_position();
	engine::fvector move(0, 0);

	if (pControls.is_triggered(controls::control::left))
	{
		if (!pCollision_system.wall_collision({ pos - engine::fvector(10, 8), { 10, 8 } }))
			move.x -= get_speed();
		mFacing_direction = direction::left;
		set_cycle(character::e_cycle::left);
	}

	if (pControls.is_triggered(controls::control::right))
	{
		if (!pCollision_system.wall_collision({ pos - engine::fvector(0, 8), { 10, 8 } }))
			move.x += get_speed();
		mFacing_direction = direction::right;
		set_cycle(character::e_cycle::right);
	}

	if (pControls.is_triggered(controls::control::up))
	{
		if (!pCollision_system.wall_collision({ pos - engine::fvector(8, 16), { 16, 16 } }))
			move.y -= get_speed();
		mFacing_direction = direction::up;
		set_cycle(character::e_cycle::up);
	}

	if (pControls.is_triggered(controls::control::down))
	{
		if (!pCollision_system.wall_collision({ pos - engine::fvector(8, 0), { 16, 4 } }))
			move.y += get_speed();
		mFacing_direction = direction::down;
		set_cycle(character::e_cycle::down);
	}

	if (move != engine::fvector(0, 0))
	{
		set_position(pos + (move*pDelta));
		tick_withtype(entity::e_type::movement);
	}
	else
		stop_withtype(entity::e_type::movement);
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
// scene
// #########


scene::scene()
{
	mTilemap.set_depth(defs::TILES_DEPTH);
	add_child(mTilemap);
	add_child(mCharacters);
	add_child(mEntities);
}

collision_system&
scene::get_collision_system()
{
	return mCollision_system;
}

inline character*
scene::find_character(const std::string& pName)
{
	for (auto &i : mCharacters)
		if (i.get_name() == pName)
			return &i;
	return nullptr;
}

inline entity*
scene::find_entity(const std::string& pName)
{
	for (auto &i : mEntities)
		if (i.get_name() == pName)
			return &i;
	return nullptr;
}

void
scene::clean_scene()
{
	mTilemap.clear();
	mCollision_system.clear();
	mCharacters.clear();
	mEntities.clear();
}

util::error
scene::load_entities(tinyxml2::XMLElement * e)
{
	assert(mTexture_manager != nullptr);
	auto ele = e->FirstChildElement();
	while (ele)
	{
		std::string name = ele->Name();
		std::string att_path = ele->Attribute("path");
		engine::fvector pos = util::shortcuts::vector_float_att(ele);

		auto& ne = mEntities.add_item();
		ne.load_entity_xml(att_path, *mTexture_manager);
		ne.set_position(pos);
		get_renderer()->add_client(&ne);

		ele = ele->NextSiblingElement();
	}
	return 0;
}

util::error
scene::load_characters(tinyxml2::XMLElement * e)
{
	assert(mTexture_manager != nullptr);
	auto ele = e->FirstChildElement();
	while (ele)
	{
		std::string name = ele->Name();
		std::string att_path = ele->Attribute("path");
		engine::fvector pos = util::shortcuts::vector_float_att(ele);

		auto& ne = mCharacters.add_item();
		ne.load_entity_xml(att_path, *mTexture_manager);
		ne.set_cycle("default");
		ne.set_position(pos *32);
		get_renderer()->add_client(&ne);

		ele = ele->NextSiblingElement();
	}
	return 0;
}

entity* scene::script_add_entity(const std::string & path)
{
	assert(mTexture_manager != nullptr);
	auto& ne = mEntities.add_item();
	ne.load_entity_xml(path, *mTexture_manager);
	get_renderer()->add_client(&ne);
	return &ne;
}

entity* scene::script_add_character(const std::string & path)
{
	assert(mTexture_manager != nullptr);
	auto& nc = mCharacters.add_item();
	nc.load_entity_xml(path, *mTexture_manager);
	nc.set_cycle(character::e_cycle::default);
	get_renderer()->add_client(&nc);
	return &nc;
}

void scene::script_set_position(entity* e, const engine::fvector & pos)
{
	assert(e != nullptr);
	e->set_position(engine::fvector(32, 32)*pos);
}

engine::fvector scene::script_get_position(entity * e)
{
	assert(e != nullptr);
	return e->get_position()*32;
}

void scene::script_set_direction(entity* e, int dir)
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

void scene::script_set_cycle(entity* e, const std::string& name)
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

void scene::script_start_animation(entity* e, int type)
{
	assert(e != nullptr);
	e->play_withtype(static_cast<entity::e_type>(type));
}

void scene::script_stop_animation(entity* e, int type)
{
	assert(e != nullptr);
	e->stop_withtype(static_cast<entity::e_type>(type));
}

void scene::script_set_animation(entity * e, const std::string & name)
{
	assert(e != nullptr);
	e->set_animation(name);
}


util::error scene::load_scene_xml(std::string pPath, script_system& pScript, flag_container& pFlags)
{
	assert(mTexture_manager != nullptr);

	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
		return util::error("Unable to load scene");
	auto root = doc.RootElement();

	mScene_path = pPath;

	clean_scene();

	// Load collision boxes
	auto ele_collisionboxes = root->FirstChildElement("collisionboxes");
	if (ele_collisionboxes)
		mCollision_system.load_collision_boxes(ele_collisionboxes, pFlags);

	// Load tilemap texture
	if (auto ele_tilemap_tex = root->FirstChildElement("tilemap_texture"))
	{
		auto tex = mTexture_manager->get_texture(ele_tilemap_tex->GetText());
		if (!tex)
			return util::error("Invalid tilemap texture");
		mTilemap.set_texture(*tex);
	}
	else
		return util::error("Tilemap texture is not defined");
	

	if (auto ele_entities = root->FirstChildElement("entities"))
	{
		load_entities(ele_entities);
	}

	if (auto ele_characters = root->FirstChildElement("characters"))
	{
		load_characters(ele_characters);
	}

	if (auto ele_script = root->FirstChildElement("script"))
	{
		auto path = ele_script->Attribute("path");
		if (path)
		{
			pScript.load_scene_script(path);
			pScript.setup_triggers(mCollision_system);
		}
	}

	if (auto ele_boundary = root->FirstChildElement("boundary"))
	{

	}
	
	if (auto ele_map = root->FirstChildElement("map"))
	{
		mTilemap_loader.load_tilemap_xml(ele_map);
		mTilemap_loader.update_display(mTilemap);
	}
	return 0;
}

util::error
scene::reload_scene(script_system & pScript, flag_container & pFlags)
{
	if (mScene_path.empty())
		return util::error("No scene currently loaded");
	return load_scene_xml(mScene_path, pScript, pFlags);
}

void scene::load_script_interface(script_system& pScript)
{
	pScript.add_function("entity add_entity(const string &in)", asMETHOD(scene, script_add_entity), this);
	pScript.add_function("entity add_character(const string &in)", asMETHOD(scene, script_add_character), this);
	pScript.add_function("void set_position(entity, const vec &in)", asMETHOD(scene, script_set_position), this);
	pScript.add_function("vec get_position(entity)", asMETHOD(scene, script_get_position), this);
	pScript.add_function("void set_direction(entity, int)", asMETHOD(scene, script_set_direction), this);
	pScript.add_function("void set_cycle(entity, const string &in)", asMETHOD(scene, script_set_cycle), this);
	pScript.add_function("void _start_animation(entity, int)", asMETHOD(scene, script_start_animation), this);
	pScript.add_function("void _stop_animation(entity, int)", asMETHOD(scene, script_stop_animation), this);
	pScript.add_function("void set_animation(entity, const string &in)", asMETHOD(scene, script_set_animation), this);
}

void
scene::set_texture_manager(texture_manager & pTexture_manager)
{
	mTexture_manager = &pTexture_manager;
}

void
scene::refresh_renderer(engine::renderer & pR)
{
	pR.add_client(&mTilemap);
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
	std::string type = "ERROR";
	if (msg->type == asEMsgType::asMSGTYPE_INFORMATION)
		type = "INFO";
	else if(msg->type == asEMsgType::asMSGTYPE_WARNING)
		type = "WARNING";
	std::cout << msg->section << "( " << msg->row << ", " << msg->col << " ) : "
		 << type << " : " << msg->message << "\n";
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

	mEngine->RegisterObjectBehaviour("vec", asBEHAVE_CONSTRUCT, "void f()"
		, asFUNCTION(as_default_constr<engine::fvector>), asCALL_CDECL_OBJLAST);
	mEngine->RegisterObjectBehaviour("vec", asBEHAVE_DESTRUCT, "void f()"
		, asFUNCTION(as_default_destr<engine::fvector>), asCALL_CDECL_OBJLAST);

	mEngine->RegisterObjectMethod("vec", "vec& opAssign(const vec &in)"
		, asMETHODPR(engine::fvector, operator=, (const engine::fvector&), engine::fvector&)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec& opAddAssign(const vec &in)"
		, asMETHODPR(engine::fvector, operator+=<float>, (const engine::fvector&), engine::fvector&)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec opMul(const float&in)"
		, asMETHODPR(engine::fvector, operator*<float>, (const float&), engine::fvector)
		, asCALL_THISCALL);

	mEngine->RegisterObjectProperty("vec", "float x", asOFFSET(engine::fvector, x));
	mEngine->RegisterObjectProperty("vec", "float y", asOFFSET(engine::fvector, y));
}

script_system::script_system()
{
	mEngine = asCreateScriptEngine();

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
	mEngine->DiscardModule("scene");
	mBuilder.StartNewModule(mEngine, "scene");
	mBuilder.AddSectionFromMemory("scene_commands", "#include 'data/scene_commands.as'");
	mBuilder.AddSectionFromFile(pPath.c_str());
	mBuilder.BuildModule();

	mScene_module = mBuilder.GetModule();
	auto func = mScene_module->GetFunctionByDecl("void start()");
	mCtxmgr.AddContext(mEngine, func);
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
		else if(!metadata.empty())
			std::cout << "Invalid Metadata: " << metadata << "\n";
	}
}

int
script_system::tick()
{
	mCtxmgr.ExecuteScripts();
	return 0;
}

// #########
// game
// #########

game::game()
{
	mRoot_node.add_child(mPlayer);
	mRoot_node.add_child(mScene);
	mNarrative.is_revealing();
	load_script_interface();
}

void
game::player_scene_interact()
{
	auto& collision = mScene.get_collision_system();

	{
		auto pos = mPlayer.get_position();
		auto trigger = collision.trigger_collision(pos);
		if (trigger)
		{
			trigger->get_function().call();
			trigger->get_function().add_arg_obj(0, &pos);
		}
	}

	if (mControls.is_triggered(controls::control::activate))
	{
		auto pos = mPlayer.get_activation_point();
		auto button = collision.button_collision(pos);
		if (button)
		{
			button->get_function().call();
			button->get_function().add_arg_obj(0, &pos);
		}
	}
}

entity* game::script_get_player()
{
	return &mPlayer;
}

void
game::load_script_interface()
{
	mScript.add_pointer_type("entity");

	mScript.add_function("float get_delta()", asMETHOD(game, get_delta), this);
	mScript.add_function("entity get_player()", asMETHOD(game, script_get_player), this);

	mScript.add_function("void _lockplayer(bool)", asMETHOD(player_character, set_locked), &mPlayer);

	mScript.add_function("bool _is_triggered(int)", asMETHOD(controls, is_triggered), &mControls);

	mFlags.load_script_interface(mScript);
	mNarrative.load_script_interface(mScript);
	mScene.load_script_interface(mScript);
	mBackground_music.load_script_interface(mScript);
}

float game::get_delta()
{
	return mClock.get_elapse().s();
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

	auto ele_player = ele_root->FirstChildElement("player");
	if (!ele_player)
		return "Please specify the player";
	std::string player_path = util::safe_string(ele_player->Attribute("path"));

	mPlayer.load_entity_xml(player_path, mTexture_manager);
	mPlayer.set_position({ 64, 120 });
	mPlayer.set_cycle(character::e_cycle::default);
	
	mScene.set_texture_manager(mTexture_manager);
	mScene.load_scene_xml(scene_path, mScript, mFlags);

	if (auto ele_narrative = ele_root->FirstChildElement("narrative"))
	{
		mNarrative.load_narrative_xml(ele_narrative, mTexture_manager);
	}
	return 0;
}

void
game::tick(controls& pControls)
{
	bool edit_mode = false;

	float delta = mClock.get_elapse().s();

	if (pControls.is_triggered(controls::control::reset))
	{
		std::cout << "Reloading scene...\n";
		mScene.reload_scene(mScript, mFlags);
		std::cout << "Done\n";
	}

	mControls = pControls;

	if (edit_mode)
	{
		if (pControls.is_triggered(controls::control::left))
			mRoot_node.set_position(mRoot_node.get_position() + engine::fvector( -delta, 0)*32);
		if (pControls.is_triggered(controls::control::right))
			mRoot_node.set_position(mRoot_node.get_position() + engine::fvector(delta, 0)*32);
		if (pControls.is_triggered(controls::control::up))
			mRoot_node.set_position(mRoot_node.get_position() + engine::fvector(0, -delta)*32);
		if (pControls.is_triggered(controls::control::down))
			mRoot_node.set_position(mRoot_node.get_position() + engine::fvector(0, delta)*32);
	}
	else{
		mPlayer.movement(pControls, mScene.get_collision_system(), delta);

		if (!mPlayer.is_locked())
		{
			mRoot_node.set_focus(mPlayer.get_position());
			player_scene_interact();
		}
	}

	mScript.tick();

	mClock.restart();
}

void
game::refresh_renderer(engine::renderer & pR)
{
	mScene.set_renderer(pR);
	pR.add_client(&mPlayer);
	mRoot_node.set_viewport(pR.get_size());
	mRoot_node.set_boundary({ 11*32, 11*32 });
	pR.add_client(&mNarrative);
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
	engine::fvector npos = pFocus - (mViewport * 0.5f);
	npos.x = util::clamp(pFocus.x, 0.f, mBoundary.x - mViewport.x);
	npos.y = util::clamp(pFocus.y, 0.f, mBoundary.y - mViewport.y);
	set_position(-npos);
}

// ##########
// narrative_dialog
// ##########

util::error
narrative_dialog::load_box(tinyxml2::XMLElement* e, texture_manager& tm)
{
	auto ele_box = e->FirstChildElement("box");

	std::string att_box_tex = ele_box->Attribute("tex");
	std::string att_box_atlas = ele_box->Attribute("atlas");

	auto tex = tm.get_texture(att_box_tex);
	if (!tex) return "Texture does not exist";
	mBox.set_texture(*tex, att_box_atlas);

	set_box_position(position::bottom);

	return 0;
}

util::error
narrative_dialog::load_font(tinyxml2::XMLElement* e)
{
	auto ele_font = e->FirstChildElement("font");

	std::string att_font_path = ele_font->Attribute("path");

	mFont.load(att_font_path);
	mText.set_font(mFont);
	mText.set_character_size(60);
	mText.set_scale(0.25f);

	mSelection.copy_format(mText);

	return 0;
}

narrative_dialog::narrative_dialog()
{
	mRevealing = false;
	hide_box();
	mInterval = defs::DEFAULT_DIALOG_SPEED;
	mBox.add_child(mText);
	mBox.add_child(mSelection);

	mText.set_position({ 5,5 });
	mSelection.set_position({ 50, 50 });
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

void
narrative_dialog::reveal_text(const std::string& pText, bool pAppend)
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

void
narrative_dialog::instant_text(std::string pText, bool pAppend)
{
	mRevealing = false;
	if (pAppend)
		mFull_text += pText;
	else
		mFull_text = pText;
	mText.set_text(mFull_text);
}

void
narrative_dialog::show_box()
{
	mText.set_visible(true);
	mBox.set_visible(true);
}

void
narrative_dialog::hide_box()
{
	mRevealing = false;
	mText.set_visible(false);
	mText.set_text("");
	mBox.set_visible(false);
	mSelection.set_visible(false);
}

bool rpg::narrative_dialog::is_box_open()
{
	return mBox.is_visible();
}

void
narrative_dialog::set_interval(float ms)
{
	mInterval = ms;
}

void
narrative_dialog::show_selection()
{
	if (mBox.is_visible())
	{
		mSelection.set_visible(true);
	}
}

void
narrative_dialog::hide_selection()
{
	mSelection.set_visible(false);
}

void
narrative_dialog::set_selection(const std::string & pText)
{
	mSelection.set_text(pText);
}

util::error
narrative_dialog::load_narrative_xml(tinyxml2::XMLElement* e, texture_manager& tm)
{
	load_box(e, tm);
	load_font(e);
	return 0;
}

void
narrative_dialog::load_script_interface(script_system & pScript)
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
}

int
narrative_dialog::draw(engine::renderer& pR)
{
	if (!mRevealing) return 0;

	float time = mTimer.get_elapse().ms();
	if (time >= mInterval)
	{
		mCount += (size_t)time / (size_t)mInterval;
		mCount  = util::clamp<size_t>(mCount, 0, mFull_text.size());

		std::string display(mFull_text.begin(), mFull_text.begin() + mCount);
		mText.set_text(display);

		if (mCount >= mFull_text.size())
			mRevealing = false;

		mTimer.restart();
	}
	return 0;
}

void
narrative_dialog::refresh_renderer(engine::renderer& r)
{
	r.add_client(&mBox);
	r.add_client(&mText);
	r.add_client(&mSelection);
}

// ##########
// script_function
// ##########

script_function::script_function():
	as_engine(nullptr),
	func(nullptr),
	ctx(nullptr),
	func_ctx(nullptr)
{
}

script_function::~script_function()
{
}

bool
script_function::is_running()
{
	if (!func_ctx) return false;
	if (func_ctx->GetFunction() != func
		|| func_ctx->GetState() == AS::asEXECUTION_FINISHED)
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
script_function::add_arg_obj(unsigned int index, void* ptr)
{
	assert(func_ctx != nullptr);
	assert(func != nullptr);
	if(index < func->GetParamCount())
		func_ctx->SetArgObject(index, ptr);
}

void
script_function::call()
{
	assert(as_engine != nullptr);
	assert(func != nullptr);
	assert(ctx != nullptr);
	if (!is_running())
	{
		if (func_ctx) ctx->DoneWithContext(func_ctx);
		func_ctx = ctx->AddContext(as_engine, func, true);
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

	pos.x = e->FloatAttribute("x");
	pos.y = e->FloatAttribute("y");

	fill.x = e->IntAttribute("w");
	fill.y = e->IntAttribute("h");
	fill.x = fill.x <= 0 ? 1 : fill.x; // Default 1
	fill.y = fill.y <= 0 ? 1 : fill.y;

	rotation = e->IntAttribute("r") % 4;
	collision = e->BoolAttribute("c");
}

bool
tilemap_loader::tile::is_adjacent_above(tile & a)
{
	return (
		atlas == a.atlas
		&& pos.x == a.pos.x
		&& pos.y == a.pos.y + a.fill.y
		&& fill.x == a.fill.x
		&& collision == a.collision
		);
}

bool
tilemap_loader::tile::is_adjacent_right(tile & a)
{
	return (
		atlas == a.atlas
		&& pos.y == a.pos.y
		&& pos.x + fill.x == a.pos.x
		&& fill.y == a.fill.y
		&& collision == a.collision
		);
}

tilemap_loader::tile*
tilemap_loader::find_tile(engine::fvector pos, size_t layer)
{
	for (auto &i : mTiles[layer])
	{
		if (i.pos == pos)
			return &i;
	}
	return nullptr;
}

// Uses brute force to merge adjacent tiles
void
tilemap_loader::condense_layer(std::vector<tile> &pMap)
{
	if (pMap.size() < 2)
		return;

	std::sort(pMap.begin(), pMap.end(), [](tile& a, tile& b)
	{ return (a.pos.y < b.pos.y) || ((a.pos.y == b.pos.y) && (a.pos.x < b.pos.x)); });

	std::vector<tile> nmap;

	tile ntile = pMap.front();

	bool merged = false;
	for (auto &i = pMap.begin() + 1; i != pMap.end(); i++)
	{
		// Merge adjacent tile 
		if (ntile.is_adjacent_right(*i))
		{
			ntile.fill.x += i->fill.x;
		}

		// Add tile
		else
		{
			// Merge any tile above this tile
			for (auto &j : nmap)
			{
				if (ntile.is_adjacent_above(j))
				{
					j.fill.y += ntile.fill.y;
					merged = true;
					break;
				}
			}
			if (!merged)
			{
				nmap.push_back(ntile);
			}
			merged = false;
			ntile = *i;
		}
	}
	if (!merged)
		nmap.push_back(ntile); // add last tile
	pMap = std::move(nmap);
}

void
tilemap_loader::condense_tiles()
{
	if (!mTiles.size()) return;
	for (auto &i : mTiles)
	{
		condense_layer(i.second);
	}
}

util::error
tilemap_loader::load_layer(tinyxml2::XMLElement * pEle, size_t pLayer)
{
	auto i = pEle->FirstChildElement();
	while (i)
	{
		tile ntile;
		ntile.load_xml(i, pLayer);
		mTiles[pLayer].push_back(ntile);
		i = i->NextSiblingElement();
	}
	return 0;
}

tilemap_loader::tilemap_loader()
{
	mTile_size = defs::TILE_SIZE;
}

util::error
tilemap_loader::load_tilemap_xml(tinyxml2::XMLElement *root)
{
	if (auto att_path = root->Attribute("path"))
	{
		load_tilemap_xml(util::safe_string(att_path));
	}

	auto ele_tilemap = root->FirstChildElement("layer");
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
tilemap_loader::find_tile_at(engine::fvector pPosition, size_t pLayer)
{
	for (auto &i : mTiles[pLayer])
	{
		if (engine::frect(i.pos, i.fill).is_intersect(pPosition))
		{
			return &i;
		}
	}
	return nullptr;
}

void
tilemap_loader::break_tile(engine::fvector pPosition, size_t pLayer)
{
	auto t = find_tile_at(pPosition, pLayer);
	if (!t || t->fill == engine::fvector(1, 1))
		return;
	auto atlas = t->atlas;
	auto fill = t->fill;
	t->fill = { 1, 1 };
	set_tile(pPosition, fill, pLayer, atlas, t->rotation);
}

void
tilemap_loader::generate(tinyxml2::XMLDocument& doc, tinyxml2::XMLNode * root)
{
	std::map<int, tinyxml2::XMLElement *> layers;
	
	for (auto &l : mTiles)
	{
		if (!l.second.size())
			continue;

		auto ele_layer = doc.NewElement("layer");
		ele_layer->SetAttribute("id", l.first);
		root->InsertEndChild(ele_layer);
		layers[l.first] = ele_layer;

		for (auto &i : l.second)
		{
			auto ele = doc.NewElement(i.atlas.c_str());
			ele->SetAttribute("x", i.pos.x);
			ele->SetAttribute("y", i.pos.y);
			if (i.fill.x > 1)    ele->SetAttribute("w", i.fill.x);
			if (i.fill.y > 1)    ele->SetAttribute("h", i.fill.y);
			if (i.rotation != 0) ele->SetAttribute("r", i.rotation);
			if (i.collision)     ele->SetAttribute("c", i.collision);
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
tilemap_loader::set_tile(engine::fvector pPosition, engine::fvector pFill, size_t pLayer, const std::string& pAtlas, int pRotation)
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
tilemap_loader::set_tile(engine::fvector pPosition, size_t pLayer, const std::string& pAtlas, int pRotation)
{
	tile* t = find_tile(pPosition, pLayer);
	if (!t)
	{
		mTiles[pLayer].emplace_back();
		auto &nt = mTiles[pLayer].back();
		nt.pos = pPosition;
		nt.fill = { 1, 1 };
		nt.atlas = pAtlas;
		nt.rotation = pRotation;
		nt.collision = false;
	}
	else
	{
		t->atlas = pAtlas;
		t->rotation = pRotation;
	}
	return 0;
}

void tilemap_loader::remove_tile(engine::fvector pPosition, size_t pLayer)
{
	for (auto &i = mTiles[pLayer].begin(); i != mTiles[pLayer].end(); i++)
	{
		if (i->pos == pPosition)
		{
			mTiles[pLayer].erase(i);
			break;
		}
	}
}

void
tilemap_loader::update_display(tilemap_A& tmA)
{
	tmA.clear();
	for (auto &l : mTiles)
	{
		for (auto &i : l.second)
		{
			engine::fvector off(0, 0);
			for (off.y = 0; off.y < i.fill.y; off.y++)
			{
				for (off.x = 0; off.x < i.fill.x; off.x++)
				{
					tmA.set_tile((i.pos + off)*mTile_size, i.atlas, l.first, i.rotation);
				}
			}
		}
	}
}

void tilemap_loader::clear()
{
	mTiles.clear();
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
	pScript.add_function("int _music_open(const string &in)", asMETHOD(engine::sound_stream, open), &mStream);
}

// ##########
// collision_box
// ##########

collision_box::collision_box() : valid(true)
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
// tilemap_A
// ##########

void tilemap_A::set_texture(engine::texture & pTexture)
{
	mTexture = &pTexture;
}

void tilemap_A::set_tile(engine::fvector pPosition, engine::frect pTexture_rect, int pLayer, int pRotation)
{
	auto &ntile = mLayers[pLayer].tiles[pPosition];
	ntile.mRef = mLayers[pLayer].vertices.add_sprite(pPosition, pTexture_rect, pRotation);
}

void tilemap_A::set_tile(engine::fvector pPosition, const std::string & pAtlas, int pLayer, int pRotation)
{
	set_tile(pPosition, mTexture->get_entry(pAtlas), pLayer, pRotation);
}

int tilemap_A::draw(engine::renderer & pR)
{
	if (!mTexture) return 1;
	for (auto &i : mLayers)
	{
		auto& vb = i.second.vertices;
		vb.set_texture(*mTexture);
		vb.set_position(get_exact_position());
		vb.draw(pR);
	}
	return 0;
}

void tilemap_A::update_animations()
{
	for (auto &i : mLayers)
	{
		for (auto &j : i.second.tiles)
		{
			j.second.update_animation();
		}
	}
}

void tilemap_A::clear()
{
	mLayers.clear();
}

void tilemap_A::tile::set_animation(engine::animation& pAnimation)
{
	mAnimation = &pAnimation;
	mTimer.start_timer(pAnimation.get_interval()*0.001f);
}

void tilemap_A::tile::update_animation()
{
	if (!mAnimation) return;
	if (mTimer.is_reached())
	{
		++mFrame;
		mTimer.start_timer(mAnimation->get_interval(mFrame)*0.001f);
	}
}
