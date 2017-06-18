#include <rpg/entity_manager.hpp>
#include <rpg/rpg_config.hpp>

using namespace rpg;

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

	engine.RegisterObjectType("entity", sizeof(entity_reference), AS::asOBJ_VALUE | AS::asGetTypeTraits<entity_reference>());

	// Constructors and deconstructors
	engine.RegisterObjectBehaviour("entity", AS::asBEHAVE_CONSTRUCT, "void f()"
		, AS::asFUNCTION(script_system::script_default_constructor<entity_reference>)
		, AS::asCALL_CDECL_OBJLAST);
	engine.RegisterObjectBehaviour("entity", AS::asBEHAVE_CONSTRUCT, "void f(const entity&in)"
		, AS::asFUNCTIONPR(script_system::script_constructor<entity_reference>
			, (const entity_reference&, void*), void)
		, AS::asCALL_CDECL_OBJLAST);
	engine.RegisterObjectBehaviour("entity", AS::asBEHAVE_DESTRUCT, "void f()"
		, AS::asFUNCTION(script_system::script_default_deconstructor<entity_reference>)
		, AS::asCALL_CDECL_OBJLAST);

	// Assignments
	engine.RegisterObjectMethod("entity", "entity& opAssign(const entity&in)"
		, AS::asMETHODPR(entity_reference, operator=, (const entity_reference&), entity_reference&)
		, AS::asCALL_THISCALL);

	engine.RegisterObjectMethod("entity", "bool opEquals(const entity&in) const"
		, AS::asMETHODPR(entity_reference, operator==, (const entity_reference&) const, bool)
		, AS::asCALL_THISCALL);


	// is_enabled
	engine.RegisterObjectMethod("entity", "bool is_valid() const"
		, AS::asMETHOD(entity_reference, is_valid)
		, AS::asCALL_THISCALL);

	engine.RegisterObjectMethod("entity", "void release()"
		, AS::asMETHOD(entity_reference, reset)
		, AS::asCALL_THISCALL);
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
	else if (e->get_type() == entity::type::text)
	{
		auto se = dynamic_cast<text_entity*>(e.get());
		return se->mText.get_size();
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

int entity_manager::script_get_direction(entity_reference& e)
{
	if (!check_entity(e)) return 0;
	character_entity* c = dynamic_cast<character_entity*>(e.get());
	if (!c)
	{
		util::error("Entity is not a character");
		return 0;
	}
	return static_cast<int>(c->get_direction());
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

void entity_manager::script_set_parallax(entity_reference & e, float pParallax)
{
	if (!check_entity(e)) return;
	e->set_parallax(pParallax);
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

void entity_manager::script_dialog_set_wordwrap(entity_reference & e, unsigned int pLength)
{
	if (!check_entity(e)) return;
	auto te = dynamic_cast<dialog_text_entity*>(e.get());
	if (!te)
	{
		util::warning("Entity is not a dialog_text_entity");
		return;
	}

	te->set_wordwrap(pLength);
}

void entity_manager::script_dialog_set_max_lines(entity_reference & e, unsigned int pLines)
{
	if (!check_entity(e)) return;
	auto te = dynamic_cast<dialog_text_entity*>(e.get());
	if (!te)
	{
		util::warning("Entity is not a dialog_text_entity");
		return;
	}

	te->set_max_lines(pLines);
}

void entity_manager::load_script_interface(script_system& pScript)
{
	mScript_system = &pScript;

	register_entity_type(pScript);

	pScript.add_function("entity add_entity(const string &in)",                      AS::asMETHOD(entity_manager, script_add_entity), this);
	pScript.add_function("entity add_entity(const string &in, const string &in)",    AS::asMETHOD(entity_manager, script_add_entity_atlas), this);
	pScript.add_function("entity add_text()",                                        AS::asMETHOD(entity_manager, script_add_text), this);
	pScript.add_function("entity add_character(const string &in)",                   AS::asMETHOD(entity_manager, script_add_character), this);

	pScript.add_function("void set_position(entity&in, const vec &in)",              AS::asMETHOD(entity_manager, script_set_position), this);
	pScript.add_function("vec get_position(entity&in)",                              AS::asMETHOD(entity_manager, script_get_position), this);
	pScript.add_function("vec get_size(entity&in)",                                  AS::asMETHOD(entity_manager, script_get_size), this);
	pScript.add_function("void _set_direction(entity&in, int)",                      AS::asMETHOD(entity_manager, script_set_direction), this);
	pScript.add_function("int _get_direction(entity&in)",                            AS::asMETHOD(entity_manager, script_get_direction), this);
	pScript.add_function("void set_cycle(entity&in, const string &in)",              AS::asMETHOD(entity_manager, script_set_cycle), this);
	pScript.add_function("void set_atlas(entity&in, const string &in)",              AS::asMETHOD(entity_manager, script_set_atlas), this);
	pScript.add_function("bool is_character(entity&in)",                             AS::asMETHOD(entity_manager, script_is_character), this);
	pScript.add_function("void remove_entity(entity&in)",                            AS::asMETHOD(entity_manager, script_remove_entity), this);
	pScript.add_function("void _set_depth_direct(entity&in, float)",                 AS::asMETHOD(entity_manager, script_set_depth_direct), this);
	pScript.add_function("void set_depth(entity&in, float)",                         AS::asMETHOD(entity_manager, script_set_depth), this);
	pScript.add_function("void set_depth_fixed(entity&in, bool)",                    AS::asMETHOD(entity_manager, script_set_depth_fixed), this);
	pScript.add_function("void _set_anchor(entity&in, int)",                         AS::asMETHOD(entity_manager, script_set_anchor), this);
	pScript.add_function("void set_rotation(entity&in, float)",                      AS::asMETHOD(entity_manager, script_set_rotation), this);
	pScript.add_function("float get_rotation(entity&in)",                            AS::asMETHOD(entity_manager, script_get_rotation), this);
	pScript.add_function("void set_color(entity&in, int, int, int, int)",            AS::asMETHOD(entity_manager, script_set_color), this);
	pScript.add_function("void set_visible(entity&in, bool)",                        AS::asMETHOD(entity_manager, script_set_visible), this);
	pScript.add_function("void set_texture(entity&in, const string&in)",             AS::asMETHOD(entity_manager, script_set_texture), this);
	pScript.add_function("void set_text(entity&in, const string &in)",               AS::asMETHOD(entity_manager, script_set_text), this);
	pScript.add_function("void set_font(entity&in, const string &in)",               AS::asMETHOD(entity_manager, script_set_font), this);
	pScript.add_function("void set_z(entity&in, float)",                             AS::asMETHOD(entity_manager, script_set_z), this);
	pScript.add_function("float get_z(entity&in)",                                   AS::asMETHOD(entity_manager, script_get_z), this);
	pScript.add_function("void set_parallax(entity&in, float)",                      AS::asMETHOD(entity_manager, script_set_parallax), this);

	pScript.set_namespace("animation");
	pScript.add_function("void start(entity&in)",                                    AS::asMETHOD(entity_manager, script_start_animation), this);
	pScript.add_function("void stop(entity&in)",                                     AS::asMETHOD(entity_manager, script_stop_animation), this);
	pScript.add_function("void pause(entity&in)",                                    AS::asMETHOD(entity_manager, script_pause_animation), this);
	pScript.add_function("bool is_playing(entity&in)",                               AS::asMETHOD(entity_manager, script_is_animation_playing), this);
	pScript.add_function("void set_speed(entity&in, float)",                         AS::asMETHOD(entity_manager, script_set_animation_speed), this);
	pScript.add_function("float get_speed(entity&in)",                               AS::asMETHOD(entity_manager, script_get_animation_speed), this);
	pScript.reset_namespace();

	pScript.add_function("void set_scale(entity&in, const vec &in)",                 AS::asMETHOD(entity_manager, script_set_scale), this);
	pScript.add_function("float get_scale(entity&in)",                               AS::asMETHOD(entity_manager, script_get_scale), this);

	pScript.add_function("void add_child(entity&in, entity&in)",                     AS::asMETHOD(entity_manager, script_add_child), this);
	pScript.add_function("void set_parent(entity&in, entity&in)",                    AS::asMETHOD(entity_manager, script_set_parent), this);
	pScript.add_function("void detach_children(entity&in)",                          AS::asMETHOD(entity_manager, script_detach_children), this);
	pScript.add_function("void detach_parent(entity&in)",                            AS::asMETHOD(entity_manager, script_detach_parent), this);
	
	pScript.add_function("entity _add_dialog_text()",                                AS::asMETHOD(entity_manager, script_add_dialog_text), this);
	pScript.add_function("void _reveal(entity&in,const string&in, bool)",            AS::asMETHOD(entity_manager, script_reveal), this);
	pScript.add_function("bool _is_revealing(entity&in)",                            AS::asMETHOD(entity_manager, script_is_revealing), this);
	pScript.add_function("void _skip_reveal(entity&in)",                             AS::asMETHOD(entity_manager, script_skip_reveal), this);
	pScript.add_function("void _set_interval(entity&in, float)",                     AS::asMETHOD(entity_manager, script_set_interval), this);
	pScript.add_function("bool _has_displayed_new_character(entity&in)",             AS::asMETHOD(entity_manager, script_has_displayed_new_character), this);
	pScript.add_function("void _dialog_set_wordwrap(entity&in, uint)",               AS::asMETHOD(entity_manager, script_dialog_set_wordwrap), this);
	pScript.add_function("void _dialog_set_max_lines(entity&in, uint)",              AS::asMETHOD(entity_manager, script_dialog_set_max_lines), this);

	pScript.add_function("void make_gui(entity&in, float)",                          AS::asMETHOD(entity_manager, script_make_gui), this);
}