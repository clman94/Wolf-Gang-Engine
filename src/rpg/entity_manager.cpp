#include <rpg/entity_manager.hpp>
#include <rpg/rpg_config.hpp>
#include <engine/logger.hpp>

using namespace rpg;

// #########
// entity_manager
// #########

entity_manager::entity_manager()
{
}

void entity_manager::clear()
{
	mEntities.clear();
}

void entity_manager::set_resource_manager(engine::resource_manager& pResource_manager)
{
	mResource_manager = &pResource_manager;
}

void entity_manager::set_world_node(engine::node & pNode)
{
	mWorld_node = &pNode;
}

void entity_manager::set_scene_node(engine::node & pNode)
{
	mScene_node = &pNode;
}

void entity_manager::register_entity_type(script_system & pScript)
{
	pScript.add_object<entity_reference>("entity");

	pScript.add_method<entity_reference, entity_reference&, const entity_reference&>("entity", operator_method::assign, &entity_reference::operator=);
	pScript.add_method<entity_reference, bool, const entity_reference&>             ("entity", operator_method::equals, &entity_reference::operator==);

	pScript.add_method("entity", "is_valid", &entity_reference::is_valid);
	pScript.add_method("entity", "release", &entity_reference::reset);
}

bool entity_manager::check_entity(entity_reference & e)
{
	assert(mScript_system != nullptr);

	if (!e.is_valid())
	{
		logger::print(mScript_system->get_current_file()
			, mScript_system->get_current_line()
			, logger::level::warning
			, "Entity object invalid.");
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

	auto resource = mResource_manager->get_resource<engine::texture>("texture", pName);
	if (!resource)
	{
		logger::warning("Could not load texture '" + pName + "'");
		return{};
	}

	new_entity->mSprite.set_texture(resource);
	new_entity->mSprite.set_animation("default:default"); // No warning if it doesn't work at first.
	                                                      // Scripter probably has other plans.
	return *new_entity;
}

entity_reference entity_manager::script_add_entity_atlas(const std::string & path, const std::string& atlas)
{
	entity_reference new_entity = script_add_entity(path);
	if (!new_entity)
		return{}; // Error, return empty

	assert(new_entity->get_type() == entity::type::sprite);
	if (!dynamic_cast<sprite_entity*>(new_entity.get())->mSprite.set_animation(atlas))
		logger::warning("Could not load atlas entry '" + atlas + "'");
	return new_entity;
}

entity_reference entity_manager::script_add_text()
{
	assert(get_renderer() != nullptr);
	assert(mResource_manager != nullptr);

	auto font = mResource_manager->get_resource<engine::font>("font", "default");
	if (!font)
	{
		logger::warning("Could not find default font");
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
		logger::error("Entity is not text");
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

	logger::error("Could not remove entity");
}

entity_reference entity_manager::script_add_character(const std::string & pName)
{
	assert(get_renderer() != nullptr);
	assert(mResource_manager != nullptr);

	auto new_entity = create_entity<character_entity>();
	if (!new_entity)
		return entity_reference(); // Return empty on error

	auto resource = mResource_manager->get_resource<engine::texture>("texture", pName);
	if (!resource)
	{
		logger::error("Could not load texture '" + pName + "' (Entity will not have a texture)");
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
		logger::warning("Unsupported entity type");
	return{};
}

void entity_manager::script_set_direction(entity_reference& e, int dir)
{
	if (!check_entity(e)) return;
	character_entity* c = dynamic_cast<character_entity*>(e.get());
	if (!c)
	{
		logger::error("Entity is not a character");
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
		logger::error("Entity is not a character");
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
		logger::error("Entity is not a character");
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
		logger::error("Entity is not sprite-based");
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
		logger::error("Entity is not sprite-based");
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
		logger::warning("Entity is not sprite-based");
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
		logger::error("Entity is not sprite-based");
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
		logger::error("Entity is not sprite-based");
		return false;
	}
	return se->mSprite.is_playing();
}

unsigned int rpg::entity_manager::script_get_animation_frame(entity_reference & e)
{
	if (!check_entity(e)) return false;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		logger::error("Entity is not sprite-based");
		return false;
	}
	return se->mSprite.get_frame();
}

void entity_manager::script_set_atlas(entity_reference& e, const std::string & name)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		logger::error("Entity is not sprite-based");
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
		logger::error("Unsupported entity type");
}

void entity_manager::script_set_rotation(entity_reference& e, float pRotation)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		logger::error("Entity is not sprite-based");
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
		logger::error("Entity is not sprite-based");
		return 0;
	}
	return se->mSprite.get_rotation();
}

void entity_manager::script_set_color(entity_reference& e, float r, float g, float b, float a)
{
	if (!check_entity(e)) return;
	
	if (e->get_type() == entity::type::sprite)
	{
		auto se = dynamic_cast<sprite_entity*>(e.get());
		se->mSprite.set_color(engine::color(r, g, b, a).clamp());
	}
	else if (e->get_type() == entity::type::text)
	{
		auto se = dynamic_cast<text_entity*>(e.get());
		se->mText.set_color(engine::color(r, g, b, a).clamp());
	}
	else
		logger::error("Unsupported entity type");
}

void entity_manager::script_set_animation_speed(entity_reference & e, float pSpeed)
{
	if (!check_entity(e)) return;
	auto se = dynamic_cast<sprite_entity*>(e.get());
	if (!se)
	{
		logger::error("Entity is not sprite-based");
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
		logger::error("Entity is not sprite-based");
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
		logger::warning("Entity is not sprite-based");
		return;
	}
	auto texture = mResource_manager->get_resource<engine::texture>("texture", name);
	if (!texture)
	{
		logger::warning("Could not load texture '" + name + "'");
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
		logger::warning("Entity is not text-based");
		return;
	}

	auto font = mResource_manager->get_resource<engine::font>("font", pName);
	if (!font)
	{
		logger::warning("Could not load font '" + pName + "'");
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
		i->set_position(i->get_position(*mWorld_node));
		i->set_parent(*mWorld_node);
	}
}

void entity_manager::script_detach_parent(entity_reference& e)
{
	if (!check_entity(e)) return;
	if (e->get_parent() != mWorld_node
		&& e->get_parent() != mScene_node)
	{
		e->set_position(e->get_position(*mWorld_node));
		e->set_parent(*mWorld_node);
	}
}

void entity_manager::script_make_gui(entity_reference & e, float pOffset)
{
	if (!check_entity(e)) return;

	// Gui elements essentually don't stick to anything in the world.
	// So we just detach everything.

	e->set_parent(*mScene_node);


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
		logger::error("Entity is not sprite-based");
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
		logger::error("Entity is not sprite-based");
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

	auto font = mResource_manager->get_resource<engine::font>("font", "default");
	if (!font)
	{
		logger::error("Could not find default font");
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
		logger::warning("Entity is not a dialog_text_entity");
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
		logger::warning("Entity is not a dialog_text_entity");
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
		logger::warning("Entity is not a dialog_text_entity");
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
		logger::warning("Entity is not a dialog_text_entity");
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
		logger::warning("Entity is not a dialog_text_entity");
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
		logger::warning("Entity is not a dialog_text_entity");
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
		logger::warning("Entity is not a dialog_text_entity");
		return;
	}

	te->set_max_lines(pLines);
}

void entity_manager::load_script_interface(script_system& pScript)
{
	mScript_system = &pScript;

	register_entity_type(pScript);

	pScript.add_function("add_entity",                        &entity_manager::script_add_entity, this);
	pScript.add_function("add_entity",                        &entity_manager::script_add_entity_atlas, this);
	pScript.add_function("add_text",                          &entity_manager::script_add_text, this);
	pScript.add_function("add_character",                     &entity_manager::script_add_character, this);

	pScript.add_function("set_position",                      &entity_manager::script_set_position, this);
	pScript.add_function("get_position",                      &entity_manager::script_get_position, this);
	pScript.add_function("get_size",                          &entity_manager::script_get_size, this);
	pScript.add_function("_set_direction",                    &entity_manager::script_set_direction, this);
	pScript.add_function("_get_direction",                    &entity_manager::script_get_direction, this);
	pScript.add_function("set_cycle",                         &entity_manager::script_set_cycle, this);
	pScript.add_function("set_atlas",                         &entity_manager::script_set_atlas, this);
	pScript.add_function("is_character",                      &entity_manager::script_is_character, this);
	pScript.add_function("remove_entity",                     &entity_manager::script_remove_entity, this);
	pScript.add_function("_set_depth_direct",                 &entity_manager::script_set_depth_direct, this);
	pScript.add_function("set_depth",                         &entity_manager::script_set_depth, this);
	pScript.add_function("set_depth_fixed",                   &entity_manager::script_set_depth_fixed, this);
	pScript.add_function("_set_anchor",                       &entity_manager::script_set_anchor, this);
	pScript.add_function("set_rotation",                      &entity_manager::script_set_rotation, this);
	pScript.add_function("get_rotation",                      &entity_manager::script_get_rotation, this);
	pScript.add_function("set_color",                         &entity_manager::script_set_color, this);
	pScript.add_function("set_visible",                       &entity_manager::script_set_visible, this);
	pScript.add_function("set_texture",                       &entity_manager::script_set_texture, this);
	pScript.add_function("set_text",                          &entity_manager::script_set_text, this);
	pScript.add_function("set_font",                          &entity_manager::script_set_font, this);
	pScript.add_function("set_z",                             &entity_manager::script_set_z, this);
	pScript.add_function("get_z",                             &entity_manager::script_get_z, this);
	pScript.add_function("set_parallax",                      &entity_manager::script_set_parallax, this);

	pScript.set_namespace("animation");
	pScript.add_function("start",                             &entity_manager::script_start_animation, this);
	pScript.add_function("stop",                              &entity_manager::script_stop_animation, this);
	pScript.add_function("pause",                             &entity_manager::script_pause_animation, this);
	pScript.add_function("is_playing",                        &entity_manager::script_is_animation_playing, this);
	pScript.add_function("set_speed",                         &entity_manager::script_set_animation_speed, this);
	pScript.add_function("get_speed",                         &entity_manager::script_get_animation_speed, this);
	pScript.add_function("get_frame",                         &entity_manager::script_get_animation_frame, this);
	pScript.reset_namespace();

	pScript.add_function("set_scale",                         &entity_manager::script_set_scale, this);
	pScript.add_function("get_scale",                         &entity_manager::script_get_scale, this);

	pScript.add_function("add_child",                         &entity_manager::script_add_child, this);
	pScript.add_function("set_parent",                        &entity_manager::script_set_parent, this);
	pScript.add_function("detach_children",                   &entity_manager::script_detach_children, this);
	pScript.add_function("detach_parent",                     &entity_manager::script_detach_parent, this);
	
	pScript.add_function("_add_dialog_text",                  &entity_manager::script_add_dialog_text, this);
	pScript.add_function("_reveal",                           &entity_manager::script_reveal, this);
	pScript.add_function("_is_revealing",                     &entity_manager::script_is_revealing, this);
	pScript.add_function("_skip_reveal",                      &entity_manager::script_skip_reveal, this);
	pScript.add_function("_set_interval",                     &entity_manager::script_set_interval, this);
	pScript.add_function("_has_displayed_new_character",      &entity_manager::script_has_displayed_new_character, this);
	pScript.add_function("_dialog_set_wordwrap",              &entity_manager::script_dialog_set_wordwrap, this);
	pScript.add_function("_dialog_set_max_lines",             &entity_manager::script_dialog_set_max_lines, this);

	pScript.add_function("make_gui",                          &entity_manager::script_make_gui, this);
}