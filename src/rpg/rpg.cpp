#include <rpg/rpg.hpp>

#include <engine/parsers.hpp>

#include "../tinyxml2/xmlshortcuts.hpp"

#include <algorithm>

using namespace rpg;
using namespace AS;

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

	auto new_entity = create_entity<sprite_entity>();
	if (!new_entity)
		return entity_reference(); // Return empty on error

	new_entity->set_texture(path, *mTexture_manager);
	new_entity->set_animation("default:default");

	return *new_entity;
}

entity_reference entity_manager::script_add_entity_atlas(const std::string & path, const std::string& atlas)
{
	entity_reference new_entity = script_add_entity(path);
	if (!new_entity)
		return entity_reference(); // Error, return empty

	dynamic_cast<sprite_entity*>(new_entity.get())->set_animation(atlas);
	return new_entity;
}

entity_reference entity_manager::script_add_text()
{
	assert(get_renderer() != nullptr);
	assert(mTexture_manager != nullptr);
	assert(mText_format != nullptr);

	auto new_entity = create_entity<text_entity>();
	if (!new_entity)
		return entity_reference(); // Return empty on error

	new_entity->apply_format(*mText_format);
	return *new_entity;
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

	auto new_entity = create_entity<character_entity>();
	if (!new_entity)
		return entity_reference(); // Return empty on error

	new_entity->set_texture(path, *mTexture_manager);
	new_entity->set_cycle(character_entity::cycle::def);

	return *new_entity;
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
	c->set_direction(static_cast<character_entity::direction>(dir));
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
		util::error("Unsupported entity type");
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
	
	if (e->get_entity_type() == entity::entity_type::sprite)
	{
		auto se = dynamic_cast<sprite_entity*>(e.get());
		se->set_color(engine::color(r, g, b, a));
	}
	else if (e->get_entity_type() == entity::entity_type::text)
	{
		auto se = dynamic_cast<text_entity*>(e.get());
		se->set_color(engine::color(r, g, b, a));
	}
	else
		util::error("Unsupported entity type");
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
	e->set_depth(defs::GUI_DEPTH - (util::clamp(pOffset, 0.f, 1000.f)/1000));
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
scene::clean(bool pFull)
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

	mPlayer.clean();

	if (pFull) // Cleanup everything
	{
		mBackground_music.clean();
	}
}

int scene::load_scene(std::string pName)
{
	assert(mTexture_manager != nullptr);
	assert(mScript != nullptr);

	clean();

	if (mLoader.load(pName))
	{
		util::error("Unable to open scene");
		return 1;
	}

	auto collision_boxes = mLoader.get_collisionboxes();
	if (collision_boxes)
		mCollision_system.load_collision_boxes(collision_boxes);

	mWorld_node.set_boundary_enable(mLoader.has_boundary());
	mWorld_node.set_boundary(mLoader.get_boundary());

	auto context = pScript_contexts[mLoader.get_name()];

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
	if (mLoader.get_name().empty())
	{
		util::error("No scene to reload");
		return 1;
	}

	clean(true);

	// Recompile Scene
	pScript_contexts[mLoader.get_name()].clean();

	return load_scene(mLoader.get_name());
}

const std::string& scene::get_path()
{
	return mLoader.get_name();
}

const std::string& scene::get_name()
{
	return mLoader.get_name();
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
	pScript.add_function("void _set_player_locked(bool)", asMETHOD(player_character, set_locked), &mPlayer);
	pScript.add_function("bool _get_player_locked()", asMETHOD(player_character, is_locked), &mPlayer);

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
	if (!ele_player ||
		!ele_player->Attribute("texture"))
	{
		util::error("Please specify the player and its texture to use.");
		return;
	}
	std::string att_texture = util::safe_string(ele_player->Attribute("texture"));

	mPlayer.set_texture(att_texture, *mTexture_manager);
	mPlayer.set_cycle(character_entity::cycle::def);

	auto ele_sounds = ele_root->FirstChildElement("sounds");
	if (ele_sounds)
		mSound_FX.load_from_directory(ele_sounds->Attribute("path"));
	else
		mSound_FX.load_from_directory(defs::DEFAULT_SOUND_PATH.string());

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

bool scene::is_ending()
{
	return mIs_ending;
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
			{
				mPlayer.set_position(*new_position);
			}
		}
	}

	if (pControls.is_triggered(controls::control::activate))
	{
		auto button = mCollision_system.button_collision(mPlayer.get_activation_point());
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

	if (pR.is_key_pressed(key_type::X) ||
		pR.is_key_pressed(key_type::RShift))
		trigger(control::back);

	if (pR.is_key_pressed(key_type::M))
		trigger(control::menu);

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
	mBuilder.AddSectionFromMemory("scene_commands", defs::INTERNAL_SCRIPTS_INCLUDE.c_str());
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
		const std::string metadata = parsers::remove_trailing_whitespace(mBuilder.GetMetadataStringForFunc(func));
		const std::string type = get_metadata_type(metadata);

		if (type == "trigger" ||
			type == "button")
		{
			trigger nt;
			script_function& sfunc = nt.get_function();
			sfunc.set_context_manager(&mScript->mCtxmgr);
			sfunc.set_engine(mScript->mEngine);
			sfunc.set_function(func);

			const std::string vectordata(metadata.begin() + type.length(), metadata.end());
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

engine::fs::path game::get_slot_path(size_t pSlot)
{
	return defs::DEFAULT_SAVES_PATH / ("slot_" + std::to_string(pSlot) + ".xml");
}

void game::save_game()
{
	const std::string path = get_slot_path(mSlot).string();
	save_system file;
	file.new_save();
	file.save_flags(mFlags);
	file.save_scene(mScene);
	file.save_player(mScene.get_player());
	file.save(path);
}

void game::open_game()
{
	const std::string path = get_slot_path(mSlot).string();
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
		mNew_scene_name = file.get_scene_path();
	}
	else
	{
		mScene.load_scene(file.get_scene_path());
	}
}

bool game::is_slot_used(size_t pSlot)
{
	const std::string path = get_slot_path(pSlot).string();
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

void game::script_load_scene(const std::string & pName)
{
	mRequest_load = true;
	mNew_scene_name = pName;
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
	if (ele_textures &&
		ele_textures->Attribute("path"))
		mTexture_manager.load_from_directory(ele_textures->Attribute("path"));
	else
		mTexture_manager.load_from_directory(defs::DEFAULT_TEXTURES_PATH.string());

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
	if (mScene.is_ending())
	{
		mScript.tick();
		return;
	}

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
		mScene.clean(true);
	}

	if (mControls.is_triggered(controls::control::editor_2))
	{
		mEditor_manager.close_editor();
		mEditor_manager.open_collisionbox_editor(mScene.get_path());
		mScene.clean(true);
	}
	if (mEditor_manager.is_editor_open())
		return;

	mScene.tick(mControls);

	mScript.tick();
	if (mRequest_load)
	{
		mRequest_load = false;
		mScene.load_scene(mNew_scene_name);
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
	mText.set_color({ 255, 255, 255, 255 });
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

	mBox.set_dynamic_depth(false);
	mBox.set_depth(defs::NARRATIVE_BOX_DEPTH);

	mText.set_dynamic_depth(false);
	mText.set_depth(defs::NARRATIVE_TEXT_DEPTH);
	mText.set_parent(mBox);

	mSelection.set_depth(defs::NARRATIVE_TEXT_DEPTH);
	mSelection.set_parent(mBox);

	mExpression.set_depth(defs::NARRATIVE_TEXT_DEPTH);
	mExpression.set_parent(mBox);

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
		mExpression_manager.load_expressions_xml(ele_expressions, pTexture_manager);
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
// background_music
// ##########

background_music::background_music()
{
	mRoot_directory = defs::DEFAULT_MUSIC_PATH;
}

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

void background_music::set_root_directory(const std::string & pPath)
{
	mRoot_directory = pPath;
}

int background_music::script_music_open(const std::string & pName)
{
	const engine::fs::path file(mRoot_directory / (pName + ".ogg"));
	if (mPath != file)
	{
		mPath = file;
		return mStream.open(file.string());
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

void text_entity::set_color(engine::color pColor)
{
	mText.set_color(pColor);
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

const text_format_profile& game_service::get_font_format() const
{
	return mFont_format;
}

engine::fvector game_service::get_tile_size() const
{
	return mTile_size;
}

int game_service::load_xml(const std::string& pPath)
{
	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
	{
		util::error("Could not load game file at '" + pPath + "'");
		return 1;
	}
	auto ele_root = doc.RootElement();

	if (auto ele_font = ele_root->FirstChildElement("font"))
	{
		mFont_format.load_settings(ele_font);
	}

	if (auto ele_font = ele_root->FirstChildElement("tile"))
	{
		mTile_size.x = ele_font->FloatAttribute("w");
		mTile_size.y = ele_font->FloatAttribute("h");
	}
	else
	{
		mTile_size = { 32.f, 32.f }; // Default
	}

	return 0;
}
