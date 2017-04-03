#include <rpg/rpg.hpp>

#include <engine/parsers.hpp>

#include "../xmlshortcuts.hpp"

#include <algorithm>
#include <fstream>

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

	engine.RegisterObjectMethod("entity", "bool opEquals(const entity&in) const"
		, asMETHODPR(entity_reference, operator==, (const entity_reference&) const, bool)
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
		return{}; // Return empty on error

	auto resource = mResource_manager->get_resource<engine::texture>(engine::resource_type::texture, pName);
	if (!resource)
	{
		util::warning("Could not load texture '" + pName + "'");
		return{};
	}

	new_entity->mSprite.set_texture(resource);
	new_entity->mSprite.set_animation("default:default"); // No warning because scripter might set it later

	return *new_entity;
}

entity_reference entity_manager::script_add_entity_atlas(const std::string & path, const std::string& atlas)
{
	entity_reference new_entity = script_add_entity(path);
	if (!new_entity)
		return{}; // Error, return empty

	assert(new_entity->get_type() == entity::type::sprite);
	if (!dynamic_cast<sprite_entity*>(new_entity.get())->mSprite.set_animation(atlas))
		util::warning("Could not load atlas entry '" + atlas + "'");
	return new_entity;
}

entity_reference entity_manager::script_add_text()
{
	assert(get_renderer() != nullptr);
	assert(mResource_manager != nullptr);

	auto font = mResource_manager->get_resource<engine::font>(engine::resource_type::font, "default");
	if (!font)
	{
		util::warning("Could not find default font");
		return{};
	}

	auto new_entity = create_entity<text_entity>();
	if (!new_entity)
		return{}; // Return empty on error

	new_entity->mText.set_font(font);
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
	c->mText.set_text(pText);
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
		new_entity->mSprite.set_texture(resource);
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
	if (!check_entity(e)) return{};
	return e->get_position();
}

engine::fvector entity_manager::script_get_size(entity_reference & e)
{
	if (!check_entity(e)) return{};

	if (e->get_type() == entity::type::sprite)
	{
		auto se = dynamic_cast<sprite_entity*>(e.get());
		return se->mSprite.get_size();
	}
	else
		util::warning("Unsupported entity type");
	return{};
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

void entity_manager::script_set_depth_direct(entity_reference & e, float pDepth)
{
	if (!check_entity(e)) return;
	e->set_depth(pDepth);
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
	se->mSprite.start();
}

void entity_manager::script_stop_animation(entity_reference& e)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::warning("Entity is not sprite-based");
		return;
	}
	se->mSprite.stop();
}

void entity_manager::script_pause_animation(entity_reference & e)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return;
	}
	se->mSprite.pause();
}

bool entity_manager::script_is_animation_playing(entity_reference & e)
{
	if (!check_entity(e)) return false;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return false;
	}
	return se->mSprite.is_playing();
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
	se->mSprite.set_animation(name);
}

void entity_manager::script_set_anchor(entity_reference& e, int pAnchor)
{
	if (!check_entity(e)) return;

	if (e->get_type() == entity::type::sprite)
	{
		auto se = dynamic_cast<sprite_entity*>(e.get());
		se->mSprite.set_anchor(static_cast<engine::anchor>(pAnchor));
	}

	else if (e->get_type() == entity::type::text)
	{
		auto se = dynamic_cast<text_entity*>(e.get());
		se->mText.set_anchor(static_cast<engine::anchor>(pAnchor));
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
	se->mSprite.set_rotation(std::fmod(std::abs(pRotation), 360.f));
}

float entity_manager::script_get_rotation(entity_reference & e)
{
	if (!check_entity(e)) return 0;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return 0;
	}
	return se->mSprite.get_rotation();
}

void entity_manager::script_set_color(entity_reference& e, int r, int g, int b, int a)
{
	if (!check_entity(e)) return;
	
	if (e->get_type() == entity::type::sprite)
	{
		auto se = dynamic_cast<sprite_entity*>(e.get());
		se->mSprite.set_color(engine::color(r, g, b, a));
	}
	else if (e->get_type() == entity::type::text)
	{
		auto se = dynamic_cast<text_entity*>(e.get());
		se->mText.set_color(engine::color(r, g, b, a));
	}
	else
		util::error("Unsupported entity type");
}

void entity_manager::script_set_animation_speed(entity_reference & e, float pSpeed)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return;
	}
	se->mSprite.set_speed(pSpeed);
}

float entity_manager::script_get_animation_speed(entity_reference & e)
{
	if (!check_entity(e)) return 0;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return 0;
	}
	return se->mSprite.get_speed();
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
	se->mSprite.set_texture(texture);
	se->mSprite.set_animation("default:default");
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
	
	te->mText.set_font(font);
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

	// Gui elements essentually don't stick to anything in the world.
	// So we just detach everything.

	e->detach_parent();


	e->set_dynamic_depth(false);
	e->set_depth(defs::GUI_DEPTH - (util::clamp(pOffset, 0.f, 1000.f)/1000));
}

bool entity_manager::script_is_character(entity_reference& e)
{
	if (!check_entity(e)) return false;
	return dynamic_cast<character_entity*>(e.get()) != nullptr;
}

void entity_manager::script_set_scale(entity_reference & e, const engine::fvector & pScale)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return;
	}
	se->mSprite.set_scale(pScale);
}

engine::fvector entity_manager::script_get_scale(entity_reference & e)
{
	if (!check_entity(e)) return{};
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		util::error("Entity is not sprite-based");
		return{};
	}
	return se->mSprite.get_scale();
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

entity_reference entity_manager::script_add_dialog_text()
{
	assert(get_renderer() != nullptr);
	assert(mResource_manager != nullptr);

	auto font = mResource_manager->get_resource<engine::font>(engine::resource_type::font, "default");
	if (!font)
	{
		util::error("Could not find default font");
		return{};
	}

	auto new_entity = create_entity<dialog_text_entity>();
	if (!new_entity)
		return{}; // Return empty on error

	new_entity->mText.set_font(font);
	return *new_entity;
}

void entity_manager::script_reveal(entity_reference & e, const std::string& pText, bool pAppend)
{
	if (!check_entity(e)) return;
	auto te = dynamic_cast<dialog_text_entity*>(e.get());
	if (!te)
	{
		util::warning("Entity is not a dialog_text_entity");
		return;
	}

	te->reveal(pText, pAppend);
}

bool entity_manager::script_is_revealing(entity_reference & e)
{
	if (!check_entity(e)) return false;
	auto te = dynamic_cast<dialog_text_entity*>(e.get());
	if (!te)
	{
		util::warning("Entity is not a dialog_text_entity");
		return false;
	}
	
	return te->is_revealing();
}

void entity_manager::script_skip_reveal(entity_reference & e)
{
	if (!check_entity(e)) return;
	auto te = dynamic_cast<dialog_text_entity*>(e.get());
	if (!te)
	{
		util::warning("Entity is not a dialog_text_entity");
		return;
	}
	te->skip_reveal();
}

void entity_manager::script_set_interval(entity_reference & e, float pMilli)
{
	if (!check_entity(e)) return;
	auto te = dynamic_cast<dialog_text_entity*>(e.get());
	if (!te)
	{
		util::warning("Entity is not a dialog_text_entity");
		return;
	}

	te->set_interval(pMilli);
}

bool entity_manager::script_has_displayed_new_character(entity_reference & e)
{
	if (!check_entity(e)) return false;
	auto te = dynamic_cast<dialog_text_entity*>(e.get());
	if (!te)
	{
		util::warning("Entity is not a dialog_text_entity");
		return false;
	}

	return te->has_revealed_character();
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
	pScript.add_function("vec get_size(entity&in)",                                  asMETHOD(entity_manager, script_get_size), this);
	pScript.add_function("void set_direction(entity&in, int)",                       asMETHOD(entity_manager, script_set_direction), this);
	pScript.add_function("void set_cycle(entity&in, const string &in)",              asMETHOD(entity_manager, script_set_cycle), this);
	pScript.add_function("void set_atlas(entity&in, const string &in)",              asMETHOD(entity_manager, script_set_atlas), this);
	pScript.add_function("bool is_character(entity&in)",                             asMETHOD(entity_manager, script_is_character), this);
	pScript.add_function("void remove_entity(entity&in)",                            asMETHOD(entity_manager, script_remove_entity), this);
	pScript.add_function("void _set_depth_direct(entity&in, float)",                 asMETHOD(entity_manager, script_set_depth_direct), this);
	pScript.add_function("void set_depth(entity&in, float)",                         asMETHOD(entity_manager, script_set_depth), this);
	pScript.add_function("void set_depth_fixed(entity&in, bool)",                    asMETHOD(entity_manager, script_set_depth_fixed), this);
	pScript.add_function("void _set_anchor(entity&in, int)",                         asMETHOD(entity_manager, script_set_anchor), this);
	pScript.add_function("void set_rotation(entity&in, float)",                      asMETHOD(entity_manager, script_set_rotation), this);
	pScript.add_function("float get_rotation(entity&in)",                            asMETHOD(entity_manager, script_get_rotation), this);
	pScript.add_function("void set_color(entity&in, int, int, int, int)",            asMETHOD(entity_manager, script_set_color), this);
	pScript.add_function("void set_visible(entity&in, bool)",                        asMETHOD(entity_manager, script_set_visible), this);
	pScript.add_function("void set_texture(entity&in, const string&in)",             asMETHOD(entity_manager, script_set_texture), this);
	pScript.add_function("void set_text(entity&in, const string &in)",               asMETHOD(entity_manager, script_set_text), this);
	pScript.add_function("void set_font(entity&in, const string &in)",               asMETHOD(entity_manager, script_set_font), this);
	pScript.add_function("void set_z(entity&in, float)",                             asMETHOD(entity_manager, script_set_z), this);
	pScript.add_function("float get_z(entity&in)",                                   asMETHOD(entity_manager, script_get_z), this);

	pScript.set_namespace("animation");
	pScript.add_function("void start(entity&in)",                                    asMETHOD(entity_manager, script_start_animation), this);
	pScript.add_function("void stop(entity&in)",                                     asMETHOD(entity_manager, script_stop_animation), this);
	pScript.add_function("void pause(entity&in)",                                    asMETHOD(entity_manager, script_pause_animation), this);
	pScript.add_function("bool is_playing(entity&in)",                               asMETHOD(entity_manager, script_is_animation_playing), this);
	pScript.add_function("void set_speed(entity&in, float)",                         asMETHOD(entity_manager, script_set_animation_speed), this);
	pScript.add_function("float get_speed(entity&in)",                               asMETHOD(entity_manager, script_get_animation_speed), this);
	pScript.reset_namespace();

	pScript.add_function("void set_scale(entity&in, float)",                         asMETHOD(entity_manager, script_set_scale), this);
	pScript.add_function("float get_scale(entity&in)",                               asMETHOD(entity_manager, script_get_scale), this);

	pScript.add_function("void add_child(entity&in, entity&in)",                     asMETHOD(entity_manager, script_add_child), this);
	pScript.add_function("void set_parent(entity&in, entity&in)",                    asMETHOD(entity_manager, script_set_parent), this);
	pScript.add_function("void detach_children(entity&in)",                          asMETHOD(entity_manager, script_detach_children), this);
	pScript.add_function("void detach_parent(entity&in)",                            asMETHOD(entity_manager, script_detach_parent), this);
	
	pScript.add_function("entity _add_dialog_text()",                                asMETHOD(entity_manager, script_add_dialog_text), this);
	pScript.add_function("void _reveal(entity&in,const string&in, bool)",            asMETHOD(entity_manager, script_reveal), this);
	pScript.add_function("bool _is_revealing(entity&in)",                            asMETHOD(entity_manager, script_is_revealing), this);
	pScript.add_function("void _skip_reveal(entity&in)",                             asMETHOD(entity_manager, script_skip_reveal), this);
	pScript.add_function("void _set_interval(entity&in, float)",                     asMETHOD(entity_manager, script_set_interval), this);
	pScript.add_function("bool _has_displayed_new_character(entity&in)",             asMETHOD(entity_manager, script_has_displayed_new_character), this);

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

	mEnd_functions.clear();

	mTilemap_display.clean();
	mTilemap_manipulator.clean();
	mCollision_system.clean();
	mEntity_manager.clean();
	mColored_overlay.clean();
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
		context.clean_globals();
		mCollision_system.setup_script_defined_triggers(context);
		context.start_all_with_tag("start");
		mEnd_functions = context.get_all_with_tag("door");
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

bool scene::create_scene(const std::string & pName)
{
	const auto xml_path = defs::DEFAULT_SCENES_PATH / (pName + ".xml");
	const auto script_path = defs::DEFAULT_SCENES_PATH / (pName + ".as");

	if (engine::fs::exists(xml_path) || engine::fs::exists(script_path))
	{
		util::error("Scene '" + pName + "' already exists");
		return false;
	}

	// Ensure the existance of the directories
	engine::fs::create_directories(xml_path.parent_path());

	// Create xml file
	std::ofstream xml(xml_path.string().c_str());
	xml << defs::MINIMAL_XML_SCENE;
	xml.close();

	// Create script file
	std::ofstream script(script_path.string().c_str());
	script << defs::MINIMAL_SCRIPT_SCENE;
	script.close();

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

	pScript.add_function("vec get_display_size()",          asMETHOD(scene, script_get_display_size), this);

	mScript = &pScript;
}

void scene::load_terminal_interface(engine::terminal_system & pTerminal)
{
	mTerminal_cmd_group = std::make_shared<engine::terminal_command_group>();

	mTerminal_cmd_group->set_root_command("scene");

	mTerminal_cmd_group->add_command("reload",
		[&](const std::vector<engine::terminal_argument>& pArgs)->bool
	{
		return reload_scene();
	}, "- Reload the scene");

	mTerminal_cmd_group->add_command("load",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.size() <= 0)
		{
			util::error("Not enough arguments");
			util::info("scene load <scene_name>");
			return false;
		}

		return load_scene(pArgs[0]);
	}, "<Scene Name> - Load a scene by name");

	mTerminal_cmd_group->add_command("new",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.size() <= 0)
		{
			util::error("Not enough arguments");
			util::info("scene new <scene_name>");
			return false;
		}

		if (!create_scene(pArgs[0]))
			return false;

		return load_scene(pArgs[0]);
	}, "<Scene Name> - Create a new scene");

	pTerminal.add_group(mTerminal_cmd_group);
}

void scene::set_resource_manager(engine::resource_manager& pResource_manager)
{
	mResource_manager = &pResource_manager;
	mEntity_manager.set_resource_manager(pResource_manager);
}

void scene::load_game_xml(tinyxml2::XMLElement * ele_root)
{
	assert(ele_root != nullptr);

	util::info("Loading XML settings...");

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

	mPlayer.mSprite.set_texture(texture);
	mPlayer.set_cycle(character_entity::cycle::def);

	util::info("Settings loaded");
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

	mSound_FX.spawn(sound, pVolume, pPitch);
}

engine::fvector scene::script_get_display_size()
{
	assert(get_renderer() != nullptr);
	return get_renderer()->get_target_size();
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
			if (mEnd_functions.empty()) // No end functions to call
			{
				load_scene(hit_door->get_scene(), hit_door->get_destination());
			}
			else if (!mEnd_functions[0].is_running())
			{
				mScript->abort_all();
				for (auto& i : mEnd_functions)
				{
					if (i.call())
					{
						i.set_arg(0, (void*)&hit_door->get_scene());
						i.set_arg(1, (void*)&hit_door->get_destination());
					}
				}
			}
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

scene_script_context::scene_script_context() :
	mScene_module(nullptr)
{}

scene_script_context::~scene_script_context()
{
}

void scene_script_context::set_path(const std::string & pFilepath)
{
	mScript_path = pFilepath;
}

bool scene_script_context::load()
{
	assert(!mScript_path.empty());
	return build_script(mScript_path);
}

bool scene_script_context::unload()
{
	clean();
	return true;
}

void scene_script_context::set_script_system(script_system & pScript)
{
	mScript = &pScript;
}

bool scene_script_context::build_script(const std::string & pPath)
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

std::string scene_script_context::get_metadata_type(const std::string & pMetadata)
{
	for (auto i = pMetadata.begin(); i != pMetadata.end(); i++)
	{
		if (!parsers::is_letter(*i))
			return std::string(pMetadata.begin(), i);
	}
	return pMetadata;
}

bool scene_script_context::is_valid() const
{
	return mScene_module.has_value();
}

void scene_script_context::clean()
{
	mTrigger_functions.clear();

	mWall_group_functions.clear();

	if (mScene_module)
	{
		mScene_module->Discard();
		mScene_module = nullptr;
	}
}

void scene_script_context::start_all_with_tag(const std::string & pTag)
{
	auto funcs = get_all_with_tag(pTag);

	for (auto& i : funcs)
	{
		i.call();
	}
}

std::vector<script_function> scene_script_context::get_all_with_tag(const std::string & pTag)
{
	std::vector<script_function> ret;
	size_t func_count = mScene_module->GetFunctionCount();
	for (size_t i = 0; i < func_count; i++)
	{
		auto func = mScene_module->GetFunctionByIndex(i);
		std::string metadata = parsers::remove_trailing_whitespace(mBuilder.GetMetadataStringForFunc(func));
		if (metadata == pTag)
		{
			script_function sfunc;
			sfunc.set_function(func);
			sfunc.set_script_system(*mScript);
			ret.push_back(sfunc);
		}
	}
	return ret;
}

void scene_script_context::clean_globals()
{
	mScene_module->ResetGlobalVars();
}

const std::vector<scene_script_context::wall_group_function>& scene_script_context::get_wall_group_functions() const
{
	return mWall_group_functions;
}


void scene_script_context::parse_wall_group_functions()
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
			if (metadata == type) // There is no specified group name
			{
				util::warning("Group name is not specified");
			}

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
	load_terminal_interface();
	mScene.load_terminal_interface(mTerminal_system);
	mEditor_manager.load_terminal_interface(mTerminal_system);
	mTerminal_gui.set_terminal_system(mTerminal_system);
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
	mScene_load_request.set_player_position(pPosition);
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

void game::load_terminal_interface()
{
	mGroup_flags = std::make_shared<engine::terminal_command_group>();
	mGroup_flags->set_root_command("flags");
	mGroup_flags->add_command("set",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.empty())
		{
			util::error("Not enough arguments");
			util::warning("flags set <Flag>");
			return false;
		}
		mFlags.set_flag(pArgs[0]);
		util::info("Flag '" + pArgs[0].get_raw() + "' has been set");
		return true;
	}, "<Flag> - Create flag");

	mGroup_flags->add_command("unset",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.empty())
		{
			util::error("Not enough arguments");
			util::warning("flags unset <Flag>");
			return false;
		}
		mFlags.unset_flag(pArgs[0]);
		util::info("Flag '" + pArgs[0].get_raw() + "' has been unset");
		return true;
	}, "<Flag> - Remove flag");

	mGroup_flags->add_command("clear",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		mFlags.clean();
		util::info("All flags cleared");
		return true;
	}, "- Remove all flags");

	mGroup_game = std::make_shared<engine::terminal_command_group>();
	mGroup_game->set_root_command("game");
	mGroup_game->add_command("reset",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		mScene.clean(true);
		return mScene.load_scene(mStart_scene);
	}, "- Reset game");

	mGroup_global1 = std::make_shared<engine::terminal_command_group>();
	mGroup_global1->add_command("help",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		std::cout << mTerminal_system.generate_help();
		return true;
	}, "- Display this help");

	mTerminal_system.add_group(mGroup_flags);
	mTerminal_system.add_group(mGroup_game);
	mTerminal_system.add_group(mGroup_global1);
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
	mTerminal_gui.update(*get_renderer());
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
	mTerminal_gui.load_gui(pR);
	pR.add_object(mEditor_manager);
	pR.set_icon("data/icon.png");
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
	mOverlay_path.clear();
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
	mOverlay_path = file;
	return 0;
}

void background_music::script_music_stop_transition_play()
{
	mStream->stop();
	mStream.swap(mOverlap_stream);
	mPath.swap(mOverlay_path);
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

	return draw_text(pR);
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
}

int text_entity::draw(engine::renderer & pR)
{
	return draw_text(pR);
}

int text_entity::draw_text(engine::renderer & pR)
{
	update_depth();
	mText.set_unit(get_unit());
	mText.set_position(get_absolute_position() - engine::fvector(0, get_z()));
	mText.draw(pR);
	return 0;
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

scenes_directory::scenes_directory()
{
	mPath = defs::DEFAULT_SCENES_PATH.string();
}

bool scenes_directory::load(engine::resource_manager & pResource_manager)
{
	if (!engine::fs::exists(mPath))
	{
		util::error("Scenes directory does not exist");
		return false;
	}

	for (const auto& i : engine::fs::recursive_directory_iterator(mPath))
	{
		const auto& script_path = i.path();

		if (script_path.extension().string() == ".xml")
		{
			std::shared_ptr<scene_script_context> context(new scene_script_context);
			//context->set_path(script_path.parent_path() + (script_path.stem().string() + ".as"));
			//pResource_manager.add_resource(engine::resource_type::script, , context);
		}
	}
	return true;
}

void scenes_directory::set_path(const std::string & pFilepath)
{
	mPath = pFilepath;
}

void scenes_directory::set_script_system(script_system & pScript)
{
	mScript = &pScript;
}

terminal_gui::terminal_gui()
{
	mEb_input = std::make_shared<tgui::EditBox>();
	mEb_input->hide();
}

void terminal_gui::set_terminal_system(engine::terminal_system & pTerminal_system)
{
	mEb_input->connect("ReturnKeyPressed",
		[&](sf::String pText)
	{
		if (!pTerminal_system.execute(pText))
		{
			util::error("Command failed '" + std::string(pText) + "'");
		}
		mEb_input->setText("");
		//mHistory.push_back(std::string(pText));
		//mCurrent_history_entry = mHistory.size();
	});
}

void terminal_gui::load_gui(engine::renderer & pR)
{
	mEb_input->setPosition("&.width - width", "&.height - height");
	pR.get_tgui().add(mEb_input);
}

void terminal_gui::update(engine::renderer& pR)
{
	if (pR.is_key_down(engine::renderer::key_type::LControl, true) // Toggle visibility
		&& pR.is_key_pressed(engine::renderer::key_type::T, true))
	{
		if (mEb_input->isVisible())
			mEb_input->hide();
		else
		{
			mEb_input->show();
			mEb_input->focus();
		}
	}

	// History is still a little buggy will fix later
	/*if (mEb_input->isFocused())
	{
		if (pR.is_key_pressed(engine::renderer::key_type::Up, true)
			&& mCurrent_history_entry >= 1)
		{
			--mCurrent_history_entry;
			mEb_input->setText(mHistory[mCurrent_history_entry]);
		}

		if (pR.is_key_pressed(engine::renderer::key_type::Down, true)
			&& mCurrent_history_entry < mHistory.size())
		{
			++mCurrent_history_entry;
			if (mCurrent_history_entry == mHistory.size())
				mEb_input->setText("");
			else
				mEb_input->setText(mHistory[mCurrent_history_entry]);
		}
	}*/
}
