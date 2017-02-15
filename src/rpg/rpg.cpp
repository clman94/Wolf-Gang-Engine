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
}

void entity_manager::clean()
{
	mEntities.clear();
}

void entity_manager::set_resource_manager(engine::resource_manager& pResource_manager)
{
	mResource_manager = &pResource_manager;
}

void entity_manager::set_root_node(engine::node & pNode)
{
	mRoot_node = &pNode;
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

	// is_enabled
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
		util::warning("(" + std::to_string(mScript_system->get_current_line()) + ") "
			+ "Entity object invalid.");
		return false;
	}
	return true;
}

entity_reference entity_manager::script_add_entity(const std::string & pName)
{
	assert(get_renderer() != nullptr);
	assert(mResource_manager != nullptr);

	auto new_entity = create_entity<sprite_entity>();
	if (!new_entity)
		return entity_reference(); // Return empty on error

	auto resource = mResource_manager->get_resource<engine::texture>(engine::resource_type::texture, pName);
	if (!resource)
	{
		util::error("Could not load texture '" + pName + "'");
	}

	new_entity->set_texture(resource);
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
	assert(mResource_manager != nullptr);

	auto font = mResource_manager->get_resource<engine::font>(engine::resource_type::font, "default");
	if (!font)
	{
		util::error("Could not find default font");
		return{};
	}

	auto new_entity = create_entity<text_entity>();
	if (!new_entity)
		return entity_reference(); // Return empty on error

	new_entity->set_font(font);
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

	// Find entity and remove it
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

entity_reference entity_manager::script_add_character(const std::string & pName)
{
	assert(get_renderer() != nullptr);
	assert(mResource_manager != nullptr);

	auto new_entity = create_entity<character_entity>();
	if (!new_entity)
		return entity_reference(); // Return empty on error

	auto resource = mResource_manager->get_resource<engine::texture>(engine::resource_type::texture, pName);
	if (!resource)
	{
		util::error("Could not load texture '" + pName + "' (Entity will not have a texture)");
	}
	else
	{
		new_entity->set_texture(resource);
		new_entity->set_cycle(character_entity::cycle::def);
	}
	return *new_entity;
}

void entity_manager::script_set_position(entity_reference& e, const engine::fvector & pos)
{
	if (!check_entity(e)) return;
	e->set_position(pos);
}

engine::fvector entity_manager::script_get_position(entity_reference& e)
{
	if (!check_entity(e)) return engine::fvector(0, 0);
	return e->get_position();
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

void entity_manager::script_set_atlas(entity_reference& e, const std::string & name)
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
		util::warning("Entity is not sprite-based");
		return;
	}
	auto texture = mResource_manager->get_resource<engine::texture>(engine::resource_type::texture, name);
	if (!texture)
	{
		util::warning("Could not load texture '" + name + "'");
		return;
	}
	se->set_texture(texture);
	se->set_animation("default:default");
}

void entity_manager::script_set_font(entity_reference & e, const std::string & pName)
{
	if (!check_entity(e)) return;
	auto te = dynamic_cast<text_entity*>(e.get());
	if (!te)
	{
		util::warning("Entity is not text-based");
		return;
	}

	auto font = mResource_manager->get_resource<engine::font>(engine::resource_type::font, pName);
	if (!font)
	{
		util::warning("Could not load font '" + pName + "'");
		return;
	}
	
	te->set_font(font);
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
		i->set_position(i->get_position(*mRoot_node));
		i->set_parent(*mRoot_node);
	}
}

void entity_manager::script_detach_parent(entity_reference& e)
{
	if (!check_entity(e)) return;

	e->set_position(e->get_position(*mRoot_node));
	e->set_parent(*mRoot_node);
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

bool entity_manager::script_is_character(entity_reference& e)
{
	if (!check_entity(e)) return false;
	return dynamic_cast<character_entity*>(e.get()) != nullptr;
}

void entity_manager::script_set_z(entity_reference & e, float pZ)
{
	if (!check_entity(e)) return;
	e->set_z(pZ);
}

float entity_manager::script_get_z(entity_reference & e)
{
	if (!check_entity(e)) return 0;
	return e->get_z();
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
	pScript.add_function("void set_atlas(entity&in, const string &in)",              asMETHOD(entity_manager, script_set_atlas), this);
	pScript.add_function("bool is_character(entity&in)",                             asMETHOD(entity_manager, script_is_character), this);
	pScript.add_function("void remove_entity(entity&in)",                            asMETHOD(entity_manager, script_remove_entity), this);
	pScript.add_function("void set_depth(entity&in, float)",                         asMETHOD(entity_manager, script_set_depth), this);
	pScript.add_function("void set_depth_fixed(entity&in, bool)",                    asMETHOD(entity_manager, script_set_depth_fixed), this);
	pScript.add_function("void _set_anchor(entity&in, int)",                         asMETHOD(entity_manager, script_set_anchor), this);
	pScript.add_function("void set_rotation(entity&in, float)",                      asMETHOD(entity_manager, script_set_rotation), this);
	pScript.add_function("void set_color(entity&in, int, int, int, int)",            asMETHOD(entity_manager, script_set_color), this);
	pScript.add_function("void set_visible(entity&in, bool)",                        asMETHOD(entity_manager, script_set_visible), this);
	pScript.add_function("void set_texture(entity&in, const string&in)",             asMETHOD(entity_manager, script_set_texture), this);
	pScript.add_function("void set_text(entity&in, const string &in)",               asMETHOD(entity_manager, script_set_text), this);
	pScript.add_function("void set_font(entity&in, const string &in)",               asMETHOD(entity_manager, script_set_font), this);
	pScript.add_function("void set_z(entity&in, float)",                             asMETHOD(entity_manager, script_set_z), this);
	pScript.add_function("float get_z(entity&in)",                                   asMETHOD(entity_manager, script_get_z), this);

	pScript.add_function("void add_child(entity&in, entity&in)",                     asMETHOD(entity_manager, script_add_child), this);
	pScript.add_function("void set_parent(entity&in, entity&in)",                    asMETHOD(entity_manager, script_set_parent), this);
	pScript.add_function("void detach_children(entity&in)",                          asMETHOD(entity_manager, script_detach_children), this);
	pScript.add_function("void detach_parent(entity&in)",                            asMETHOD(entity_manager, script_detach_parent), this);

	pScript.add_function("void make_gui(entity&in, float)",                          asMETHOD(entity_manager, script_make_gui), this);
}



// #########
// scene
// #########

scene::scene()
{
	mTilemap_display.set_depth(defs::TILES_DEPTH);

	// World space will be 32x32 pixels per unit
	mWorld_node.set_unit(32);

	mWorld_node.add_child(mTilemap_display);
	mWorld_node.add_child(mPlayer);
	mFocus_player = true;

	mPathfinding_system.set_collision_system(mCollision_system);

	mEntity_manager.set_root_node(mWorld_node);
}

scene::~scene()
{
	util::info("Destroying scene");
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
	mScript->abort_all();

	mTilemap_display.clean();
	mTilemap_manipulator.clean();
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

		// Clear all contexts for recompiling
		for (auto &i : pScript_contexts)
			i.second.clean();
		pScript_contexts.clear();
	}
}

bool scene::load_scene(std::string pName)
{
	assert(mResource_manager != nullptr);
	assert(mScript != nullptr);

	clean();

	mCurrent_scene_name = pName;

	util::info("Loading scene '" + pName + "'");

	if (!mLoader.load(pName))
	{
		util::error("Unable to open scene '" + pName + "'");
		return false;
	}

	auto collision_boxes = mLoader.get_collisionboxes();
	if (collision_boxes)
		mCollision_system.load_collision_boxes(collision_boxes);

	mWorld_node.set_boundary_enable(mLoader.has_boundary());
	mWorld_node.set_boundary(mLoader.get_boundary());

	auto &context = pScript_contexts[mLoader.get_name()];

	// Compile script if not already
	if (!context.is_valid())
	{
		context.set_script_system(*mScript);
		context.build_script(mLoader.get_script_path());
	}
	else
		util::info("Script is already compiled");

	// Setup context if still valid
	if (context.is_valid())
	{
		mCollision_system.setup_script_defined_triggers(context);
		context.start_all_with_tag("start");
	}

	auto tilemap_texture = mResource_manager->get_resource<engine::texture>(engine::resource_type::texture, mLoader.get_tilemap_texture());
	if (!tilemap_texture)
	{
		util::error("Invalid tilemap texture");
		return false;
	}
	mTilemap_display.set_texture(tilemap_texture);

	mTilemap_manipulator.load_tilemap_xml(mLoader.get_tilemap());
	mTilemap_manipulator.update_display(mTilemap_display);

	// Pre-execute so the scene script can setup things before the render.
	mScript->tick();

	update_focus();

	util::info("Cleaning up resources...");
	// Ensure resources are ready and unused stuff is put away
	mResource_manager->ensure_load();
	mResource_manager->unload_unused();
	util::info("Resources ready");

	return true;
}

bool scene::load_scene(std::string pName, std::string pDoor)
{
	if (!load_scene(pName))
		return false;

	auto position = mCollision_system.get_door_entry(pDoor);
	if (!position)
	{
		util::warning("Enable to find door '" + pDoor + "'");
		return false;
	}
	mPlayer.set_position(*position);

	return true;
}

bool scene::reload_scene()
{
	if (mCurrent_scene_name.empty())
	{
		util::error("No scene to reload");
		return 1;
	}

	clean(true);

	return load_scene(mCurrent_scene_name);
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
	mCollision_system.load_script_interface(pScript);

	pScript.add_function("void set_tile(const string &in, vec, int, int)", asMETHOD(scene, script_set_tile), this);
	pScript.add_function("void remove_tile(vec, int)",      asMETHOD(scene, script_remove_tile), this);

	pScript.add_function("int _spawn_sound(const string&in, float, float)", asMETHOD(scene, script_spawn_sound), this);
	pScript.add_function("void _stop_all()",                asMETHOD(engine::sound_spawner, stop_all), &mSound_FX);

	pScript.add_function("entity get_player()",             asMETHOD(scene, script_get_player), this);
	pScript.add_function("void _set_player_locked(bool)",   asMETHOD(player_character, set_locked), &mPlayer);
	pScript.add_function("bool _get_player_locked()",       asMETHOD(player_character, is_locked), &mPlayer);

	pScript.add_function("void _set_focus(vec)",            asMETHOD(scene, script_set_focus), this);
	pScript.add_function("vec _get_focus()",                asMETHOD(scene, script_get_focus), this);
	pScript.add_function("void _focus_player(bool)",        asMETHOD(scene, focus_player), this);

	pScript.add_function("vec get_boundary_position()",     asMETHOD(scene, script_get_boundary_position), this);
	pScript.add_function("vec get_boundary_size()",         asMETHOD(scene, script_get_boundary_size), this);
	pScript.add_function("void get_boundary_position(vec)", asMETHOD(scene, script_set_boundary_position), this);
	pScript.add_function("void get_boundary_size(vec)",     asMETHOD(scene, script_set_boundary_size), this);
	pScript.add_function("void set_boundary_enable(bool)",  asMETHOD(panning_node, set_boundary_enable), this);

	mScript = &pScript;
}

void scene::set_resource_manager(engine::resource_manager& pResource_manager)
{
	mResource_manager = &pResource_manager;
	mEntity_manager.set_resource_manager(pResource_manager);
}

void scene::load_game_xml(tinyxml2::XMLElement * ele_root)
{
	assert(ele_root != nullptr);

	if (auto ele_tile_size = ele_root->FirstChildElement("tile_size"))
	{
		mWorld_node.set_unit(ele_tile_size->FloatAttribute("pixels"));
	}
	else
	{
		util::error("Please specify Tile size");
		return;
	}

	auto ele_player = ele_root->FirstChildElement("player");
	if (!ele_player ||
		!ele_player->Attribute("texture"))
	{
		util::error("Please specify the player and its texture to use.");
		return;
	}
	const std::string att_texture = util::safe_string(ele_player->Attribute("texture"));

	auto texture = mResource_manager->get_resource<engine::texture>(engine::resource_type::texture, att_texture);
	if (!texture)
	{
		util::error("Could not load texture '" + att_texture + "' for player character");
		return;
	}

	mPlayer.set_texture(texture);
	mPlayer.set_cycle(character_entity::cycle::def);


	if (auto ele_narrative = ele_root->FirstChildElement("narrative"))
	{
		assert(mResource_manager != nullptr);
		mNarrative.load_narrative_xml(ele_narrative, *mResource_manager);
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


void scene::script_set_focus(engine::fvector pPosition)
{
	mFocus_player = false;
	mWorld_node.set_focus(pPosition);
}

engine::fvector scene::script_get_focus()
{
	return mWorld_node.get_focus();
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

void scene::script_spawn_sound(const std::string & pName, float pVolume, float pPitch)
{
	auto sound = mResource_manager->get_resource<engine::sound_buffer>(engine::resource_type::sound, pName);
	if (!sound)
	{
		util::error("Could not spawn sound '" + pName + "'");
		return;
	}

	mSound_FX.spawn(sound);
}

void scene::script_set_boundary_position(engine::fvector pPosition)
{
	mWorld_node.set_boundary(mWorld_node.get_boundary().set_offset(pPosition));
}

void scene::script_set_tile(const std::string& pAtlas, engine::fvector pPosition
	, int pLayer, int pRotation)
{
	mTilemap_manipulator.set_tile(pPosition, pLayer, pAtlas, pRotation);
	mTilemap_manipulator.update_display(mTilemap_display);
}

void scene::script_remove_tile(engine::fvector pPosition, int pLayer)
{
	mTilemap_manipulator.remove_tile(pPosition, pLayer);
	mTilemap_manipulator.update_display(mTilemap_display);
}

void scene::refresh_renderer(engine::renderer& pR)
{
	mWorld_node.set_viewport(pR.get_target_size());
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
	collision_box_container& container = mCollision_system.get_container();

	const auto collision_box = mPlayer.get_collision_box();

	// Check collision with triggers
	{
		auto triggers = container.collision(collision_box::type::trigger, collision_box);
		for (auto& i : triggers)
			std::dynamic_pointer_cast<trigger>(i)->call_function();
	}

	// Check collision with doors
	{
		const auto hit = container.first_collision(collision_box::type::door, collision_box);
		if (hit)
		{
			const auto hit_door = std::dynamic_pointer_cast<door>(hit);
			load_scene(hit_door->get_scene(), hit_door->get_destination());
		}
	}

	// Check collision with buttons on when "activate" is triggered
	if (pControls.is_triggered(controls::control::activate))
	{
		auto buttons = container.collision(collision_box::type::button, mPlayer.get_activation_point());
		for (auto& i : buttons)
			std::dynamic_pointer_cast<button>(i)->call_function();
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
	mControls[static_cast<size_t>(pControl)] = true;
}

bool
controls::is_triggered(control pControl)
{
	return mControls[static_cast<size_t>(pControl)];
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
		if (pR.is_key_down(key_type::LShift)
			&& pR.is_key_pressed(key_type::R))
			trigger(control::reset_game);
		else if (pR.is_key_pressed(key_type::R))
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

script_context::~script_context()
{
	
}

void script_context::set_script_system(script_system & pScript)
{
	mScript = &pScript;
}

bool script_context::build_script(const std::string & pPath)
{
	util::info("Compiling script '" + pPath + "'...");

	clean();

	mBuilder.StartNewModule(&mScript->get_engine(), pPath.c_str());
	mBuilder.AddSectionFromMemory("scene_commands", defs::INTERNAL_SCRIPTS_INCLUDE.c_str());
	mBuilder.AddSectionFromFile(pPath.c_str());
	if (mBuilder.BuildModule())
	{
		util::error("Failed to load scene script");
		return false;
	}
	mScene_module = mBuilder.GetModule();

	parse_wall_group_functions();

	util::info("Script compiled");
	return true;
}

std::string script_context::get_metadata_type(const std::string & pMetadata)
{
	for (auto i = pMetadata.begin(); i != pMetadata.end(); i++)
	{
		if (!parsers::is_letter(*i))
			return std::string(pMetadata.begin(), i);
	}
	return pMetadata;
}

bool script_context::is_valid() const
{
	return mScene_module.has_value();
}

void script_context::clean()
{
	mTrigger_functions.clear();

	mWall_group_functions.clear();

	if (mScene_module)
	{
		mScene_module->Discard();
		mScene_module = nullptr;
	}
}

void script_context::start_all_with_tag(const std::string & pTag)
{
	size_t func_count = mScene_module->GetFunctionCount();
	for (size_t i = 0; i < func_count; i++)
	{
		auto func = mScene_module->GetFunctionByIndex(i);
		std::string metadata = parsers::remove_trailing_whitespace(mBuilder.GetMetadataStringForFunc(func));
		if (metadata == pTag)
		{
			mScript->create_thread(func);
		}
	}
}

const std::vector<script_context::wall_group_function>& script_context::get_wall_group_functions() const
{
	return mWall_group_functions;
}


void script_context::parse_wall_group_functions()
{
	util::info("Binding functions to wall groups...");
	size_t func_count = mScene_module->GetFunctionCount();
	for (size_t i = 0; i < func_count; i++)
	{
		auto as_function = mScene_module->GetFunctionByIndex(i);
		const std::string metadata = parsers::remove_trailing_whitespace(mBuilder.GetMetadataStringForFunc(as_function));
		const std::string type = get_metadata_type(metadata);

		if (type == "group")
		{
			std::unique_ptr<script_function> function(new script_function);
			function->set_script_system(*mScript);
			function->set_function(as_function);

			wall_group_function wgf;
			wgf.function = function.get();

			mTrigger_functions[as_function->GetDeclaration(true, true)].swap(function);

			const std::string group(metadata.begin() + type.length() + 1, metadata.end());
			wgf.group = group;

			mWall_group_functions.push_back(wgf);
		}
	}

	util::info(std::to_string(mWall_group_functions.size()) + " function(s) bound");
}

// #########
// game
// #########

game::game()
{
	load_script_interface();
	mEditor_manager.set_resource_manager(mResource_manager);
	mEditor_manager.set_world_node(mScene.get_world_node());
	mSlot = 0;
}

game::~game()
{
	util::info("Destroying game");
	mScene.clean();
}

engine::fs::path game::get_slot_path(size_t pSlot)
{
	return defs::DEFAULT_SAVES_PATH / ("slot_" + std::to_string(pSlot) + ".xml");
}

void game::save_game()
{
	const std::string path = get_slot_path(mSlot).string();
	save_system file;

	util::info("Saving game...");
	file.new_save();
	file.save_flags(mFlags);
	file.save_scene(mScene);
	file.save_player(mScene.get_player());
	file.save(path);
	util::info("Game saved to '" + path + "'");
}

void game::open_game()
{
	const std::string path = get_slot_path(mSlot).string();
	save_system file;
	if (!file.open_save(path))
	{
		util::error("Invalid slot '" + std::to_string(mSlot) + "'");
		return;
	}
	util::info("Opening game...");
	mFlags.clean();
	file.load_flags(mFlags);
	file.load_player(mScene.get_player());
	mScript.abort_all();
	if (mScript.is_executing())
	{
		mScene_load_request.request_load(file.get_scene_path());
	}
	else
	{
		mScene.load_scene(file.get_scene_path());
	}

	util::info("Loaded " + std::to_string(mFlags.get_count()) + " flag(s)");
	util::info("Game opened from '" + path + "'");
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
	util::info("Requesting scene load '" + pName + "'");
	mScene_load_request.request_load(pName);
}

void game::script_load_scene_to_door(const std::string & pName, const std::string & pDoor)
{
	util::info("Requesting scene load '" + pName + "'");
	mScene_load_request.request_load(pName);
	mScene_load_request.set_player_position(pDoor);
}

void game::script_load_scene_to_position(const std::string & pName, engine::fvector pPosition)
{
	util::info("Requesting scene load '" + pName + "'");
	mScene_load_request.request_load(pName);
	mScene_load_request.set_player_position(pPosition * 32.f);
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
	mScript.add_function("void load_scene(const string &in, const string &in)", asMETHOD(game, script_load_scene_to_door), this);
	mScript.add_function("void load_scene(const string &in, vec)", asMETHOD(game, script_load_scene_to_position), this);

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
	mStart_scene = util::safe_string(ele_scene->Attribute("name"));


	util::info("Loading Resources...");

	// Setup textures directory
	std::shared_ptr<texture_directory> texture_dir(std::make_shared<texture_directory>());
	if (auto ele_textures = ele_root->FirstChildElement("textures"))
		texture_dir->set_path(ele_textures->Attribute("path"));
	mResource_manager.add_directory(texture_dir);

	// Setup sounds directory
	std::shared_ptr<soundfx_directory> sound_dir(std::make_shared<soundfx_directory>());
	if (auto ele_sounds = ele_root->FirstChildElement("sounds"))
		sound_dir->set_path(ele_sounds->Attribute("path"));
	mResource_manager.add_directory(sound_dir);

	// Setup fonts directory
	std::shared_ptr<font_directory> font_dir(std::make_shared<font_directory>());
	if (auto ele_fonts = ele_root->FirstChildElement("fonts"))
		font_dir->set_path(ele_fonts->Attribute("path"));
	mResource_manager.add_directory(font_dir);

	mResource_manager.reload_directories(); // Load the resources from the directories
	
	util::info("Resources loaded");

	mScene.set_resource_manager(mResource_manager);
	mScene.load_game_xml(ele_root);
	mScene.load_scene(mStart_scene);
	return 0;
}

void
game::tick()
{
	mControls.update(*get_renderer());
	if (mControls.is_triggered(controls::control::reset))
	{
		mEditor_manager.close_editor();
		util::info("Reloading scene...");

		mResource_manager.reload_directories();

		mScene.reload_scene();
		util::info("Scene reloaded");
	}

	if (mControls.is_triggered(controls::control::reset_game))
	{
		mEditor_manager.close_editor();
		util::info("Reloading entire game...");

		mResource_manager.reload_directories();

		mFlags.clean();

		mScene.clean(true);
		mScene.load_scene(mStart_scene);

		util::info("Game reloaded");
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
	// Dont go any further if editor is open
	if (mEditor_manager.is_editor_open())
		return;

	mScene.tick(mControls);

	mScript.tick();

	if (mScene_load_request.is_requested())
	{
		switch (mScene_load_request.get_player_position_type())
		{
		case scene_load_request::to_position::door:
			mScene.load_scene(mScene_load_request.get_scene_name()
				, mScene_load_request.get_player_door());
			break;

		case scene_load_request::to_position::position:
			mScene.load_scene(mScene_load_request.get_scene_name());
			mScene.get_player().set_position(mScene_load_request.get_player_position());
			break;

		case scene_load_request::to_position::none:
			mScene.load_scene(mScene_load_request.get_scene_name());
			break;
		}
		mScene_load_request.complete();
	}
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
narrative_dialog::load_box(tinyxml2::XMLElement* pEle, engine::resource_manager& pResource_manager)
{
	auto ele_box = pEle->FirstChildElement("box");

	std::string att_box_tex = ele_box->Attribute("texture");
	std::string att_box_atlas = ele_box->Attribute("atlas");

	auto texture = pResource_manager.get_resource<engine::texture>(engine::resource_type::texture, att_box_tex);
	if (!texture)
	{
		util::error("Failed to load narrative box texture");
		return 1;
	}
	mBox.set_texture(texture);
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
	auto expression = mExpression_manager.find_expression(pName);
	if (!expression)
	{
		util::error("Expression '" + pName + "' does not exist");
		return;
	}
	mExpression.set_texture(expression->texture);
	mExpression.set_animation(expression->animation);
	show_expression();
}

int narrative_dialog::load_narrative_xml(tinyxml2::XMLElement* pEle, engine::resource_manager& pResource_manager)
{
	load_box(pEle, pResource_manager);

	auto font = pResource_manager.get_resource<engine::font>(engine::resource_type::font, "default");
	if (!font)
	{
		util::error("Faled to find font");
		return 1;
	}
	mText.set_font(font);

	if (auto ele_expressions = pEle->FirstChildElement("expressions"))
		mExpression_manager.load_expressions_xml(ele_expressions, pResource_manager);
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

script_function::script_function()
{
}

script_function::~script_function()
{
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
script_function::set_function(AS::asIScriptFunction * pFunction)
{
	mFunction = pFunction;
}

void
script_function::set_script_system(script_system& pScript_system)
{
	mScript_system = &pScript_system;
}

void
script_function::set_arg(unsigned int index, void* ptr)
{
	if(index < mFunction->GetParamCount())
		func_ctx->SetArgObject(index, ptr);
}

bool
script_function::call()
{
	if (!is_running())
	{
		return_context();
		func_ctx = mScript_system->create_thread(mFunction, true);
		return true;
	}
	return false;
}

void script_function::return_context()
{
	if (func_ctx)
	{
		func_ctx->Abort();
		mScript_system->return_context(func_ctx);
		func_ctx = nullptr;
	}
}


// ##########
// background_music
// ##########

background_music::background_music()
{
	mRoot_directory = defs::DEFAULT_MUSIC_PATH;
	mStream.reset(new engine::sound_stream);
	mOverlap_stream.reset(new engine::sound_stream);
}

void background_music::load_script_interface(script_system & pScript)
{
	pScript.add_function("void _music_play()", asMETHOD(engine::sound_stream, play), mStream.get());
	pScript.add_function("void _music_stop()", asMETHOD(engine::sound_stream, stop), mStream.get());
	pScript.add_function("void _music_pause()", asMETHOD(engine::sound_stream, pause), mStream.get());
	pScript.add_function("float _music_get_position()", asMETHOD(engine::sound_stream, get_position), mStream.get());
	pScript.add_function("void _music_set_position(float)", asMETHOD(engine::sound_stream, set_position), mStream.get());
	pScript.add_function("float _music_get_volume()", asMETHOD(engine::sound_stream, get_volume), mStream.get());
	pScript.add_function("void _music_set_volume(float)", asMETHOD(engine::sound_stream, set_volume), mStream.get());
	pScript.add_function("void _music_set_loop(bool)", asMETHOD(engine::sound_stream, set_loop), mStream.get());
	pScript.add_function("int _music_open(const string &in)", asMETHOD(background_music, script_music_open), this);
	pScript.add_function("bool _music_is_playing()", asMETHOD(engine::sound_stream, is_playing), mStream.get());
	pScript.add_function("float _music_get_duration()", asMETHOD(engine::sound_stream, get_duration), mStream.get());
	pScript.add_function("int _music_swap(const string &in)", asMETHOD(background_music, script_music_swap), this);
	pScript.add_function("int _music_start_transition_play(const string &in)", asMETHOD(background_music, script_music_start_transition_play), this);
	pScript.add_function("void _music_stop_transition_play()", asMETHOD(background_music, script_music_stop_transition_play), this);
	pScript.add_function("void _music_set_second_volume(float)", asMETHOD(background_music, script_music_set_second_volume), this);
}

void background_music::clean()
{
	mStream->stop();
	mStream->set_volume(100);
	mOverlap_stream->stop();
	mPath.clear();
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
		return mStream->open(file.string());
	}
	return 0;
}

int background_music::script_music_swap(const std::string & pName)
{
	const engine::fs::path file(mRoot_directory / (pName + ".ogg"));
	if (mPath == file)
		return 0;

	if (!mStream->is_playing())
	{
		script_music_open(pName);
		return 0;
	}

	// Open a second stream and set its position similar to
	// the first one.
	mOverlap_stream->open(file.string());
	mOverlap_stream->set_loop(true);
	mOverlap_stream->set_volume(mStream->get_volume());
	mOverlap_stream->play();
	if (mOverlap_stream->get_duration() >= mStream->get_duration())
		mOverlap_stream->set_position(mStream->get_position());

	// Make the new stream the main stream
	mStream->stop();
	mStream.swap(mOverlap_stream);

	mPath = file;
	return 0;
}

int background_music::script_music_start_transition_play(const std::string & pName)
{
	const engine::fs::path file(mRoot_directory / (pName + ".ogg"));
	if (mPath == file)
		return 0;

	mOverlap_stream->open(file.string());
	mOverlap_stream->set_loop(true);
	mOverlap_stream->set_volume(0);
	mOverlap_stream->play();

	mPath = file;
	return 0;
}

void background_music::script_music_stop_transition_play()
{
	mStream->stop();
	mStream.swap(mOverlap_stream);
}

void background_music::script_music_set_second_volume(float pVolume)
{
	mOverlap_stream->set_volume(pVolume);
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
	// Create saves folder if it doesn't exist
	if (!engine::fs::exists(defs::DEFAULT_SAVES_PATH))
		engine::fs::create_directory(defs::DEFAULT_SAVES_PATH);

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

util::optional_pointer<const expression_manager::expression> expression_manager::find_expression(const std::string & mName)
{
	auto find = mExpressions.find(mName);
	if (find != mExpressions.end())
	{
		return &find->second;
	}
	return{};
}

int expression_manager::load_expressions_xml(tinyxml2::XMLElement * pRoot, engine::resource_manager& pResource_manager)
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
		auto texture = pResource_manager.get_resource<engine::texture>(engine::resource_type::texture, att_texture);
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

		mExpressions[ele_expression->Name()].texture = texture;
		mExpressions[ele_expression->Name()].animation = texture->get_entry(att_atlas)->get_animation();

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
	mOverlay.set_size(pR.get_target_size());
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
		auto hit = mCollision_system->get_container().first_collision(collision_box::type::wall, { pos, { 0.9f, 0.9f } });
		return (bool)hit;
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
	mText.set_parent(*this);
}

void text_entity::set_font(std::shared_ptr<engine::font> pFont)
{
	mText.set_font(pFont, true);
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
	mText.draw(pR);
	return 0;
}

void text_entity::update_z()
{
	mText.set_position(-engine::fvector(0, get_z()));
}

bool game_settings_loader::load(const std::string & pPath)
{
	using namespace tinyxml2;
	XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
	{
		util::error("Could not load game file at '" + pPath + "'");
		return false;
	}
	auto ele_root = doc.RootElement();

	auto ele_scene = ele_root->FirstChildElement("scene");
	if (!ele_scene)
	{
		util::error("Please specify the scene to start with");
		return false;
	}
	mStart_scene = util::safe_string(ele_scene->Attribute("name"));

	mTextures_path = load_setting_path(ele_root, "textures", defs::DEFAULT_TEXTURES_PATH.string());
	mSounds_path   = load_setting_path(ele_root, "sounds"  , defs::DEFAULT_SOUND_PATH.string());
	mMusic_path    = load_setting_path(ele_root, "music"   , defs::DEFAULT_MUSIC_PATH.string());

	return true;
}

const std::string & game_settings_loader::get_start_scene() const
{
	return mStart_scene;
}

const std::string & game_settings_loader::get_textures_path() const
{
	return mTextures_path;
}

const std::string & game_settings_loader::get_sounds_path() const
{
	return mSounds_path;
}

const std::string & game_settings_loader::get_music_path() const
{
	return mMusic_path;
}

const std::string & game_settings_loader::get_player_texture() const
{
	return mPlayer_texture;
}

std::string game_settings_loader::load_setting_path(tinyxml2::XMLElement* pRoot, const std::string & pName, const std::string& pDefault)
{
	auto ele = pRoot->FirstChildElement("textures");
	if (ele &&
		ele->Attribute("path"))
		return util::safe_string(ele->Attribute("path"));
	else
		return pDefault;
}

scene_load_request::scene_load_request()
{
	mRequested = false;
	mTo_position = to_position::none;
}

void scene_load_request::request_load(const std::string & pScene_name)
{
	mRequested = true;
	mScene_name = pScene_name;
}

bool scene_load_request::is_requested() const
{
	return mRequested;
}

const std::string & rpg::scene_load_request::get_scene_name() const
{
	return mScene_name;
}

void scene_load_request::complete()
{
	mRequested = false;
	mTo_position = to_position::none;
	mScene_name.clear();
}

void scene_load_request::set_player_position(engine::fvector pPosition)
{
	mTo_position = to_position::position;
	mPosition = pPosition;
}

void scene_load_request::set_player_position(const std::string & pDoor)
{
	mTo_position = to_position::door;
	mDoor = pDoor;
}

scene_load_request::to_position scene_load_request::get_player_position_type() const
{
	return mTo_position;
}

const std::string & scene_load_request::get_player_door() const
{
	return mDoor;
}

engine::fvector scene_load_request::get_player_position() const
{
	return mPosition;
}
