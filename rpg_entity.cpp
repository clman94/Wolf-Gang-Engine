#include "rpg_entity.hpp"
using namespace rpg;

entity::entity()
{
	for (int i = 0; i < 5; i++)
	{
		cycles[i] = nullptr;
	}
}

std::string 
entity::get_name()
{
	return name;
}

void
entity::set_name(std::string _name)
{
	name = _name;
}

entity::animation*
entity::find_animation(std::string name)
{
	for (auto &i : world)
	{
		if (i.name == name)
			return &i;
	}
	return nullptr;
}

engine::animated_sprite_node*
entity::get_animation()
{
	return &cycles[c_cycle]->node;
}

utility::error
entity::set_cycle_animation(std::string _name, cycle_type cycle)
{
	auto a = find_animation(_name);
	cycles[cycle] = nullptr;
	if (!a)
		return "Entity animation '" + _name + 
			"' in entity '" + name + "' does not exist";
	a->node.set_relative_position(a->node.get_size() * engine::fvector(0.5, 1) * (-1)); // Anchor at bottom
	cycles[cycle] = a;
	return 0;
}

int
entity::draw(engine::renderer &_r)
{
	if (c_anim)
		c_anim->node.draw(_r);
	return 0;
}

void
entity::animation_start(animation_type type, bool loop)
{
	if (c_anim && c_anim->type == type)
	{
		c_anim->node.set_loop(loop);
		c_anim->node.start();
	}
}

void
entity::animation_stop(animation_type type)
{
	if (c_anim && c_anim->type == type)
		c_anim->node.stop();
}

bool
entity::is_animation_done()
{
	if (c_anim)
		return c_anim->node.is_playing();
	return true;
}

void
entity::move_left(float delta)
{
	if (c_anim && c_anim->type == WALK)
		c_anim->node.tick_animation();
	set_relative_position(get_relative_position() + engine::fvector(-delta));
}

void
entity::move_right(float delta)
{
	if (c_anim && c_anim->type == WALK)
		c_anim->node.tick_animation();
	set_relative_position(get_relative_position() + engine::fvector(delta));
}

void
entity::move_up(float delta)
{
	if (c_anim && c_anim->type == WALK)
		c_anim->node.tick_animation();
	set_relative_position(get_relative_position() + engine::fvector(0, -delta));
}

void
entity::move_down(float delta)
{
	if (c_anim && c_anim->type == WALK)
		c_anim->node.tick_animation();
	set_relative_position(get_relative_position() + engine::fvector(0, delta));
}

void
entity::set_cycle(int cycle)
{
	c_cycle = cycle;
	if (cycles[cycle])
		c_anim = cycles[cycle];
	else
		c_anim = cycles[DEFAULT];

	animation_start(CONSTANT, true);
}

engine::fvector
entity::get_activate_point()
{
	using namespace engine;
	switch (c_cycle)
	{
	case LEFT:
		return get_relative_position() - fvector(32, 0);
	case RIGHT:
		return get_relative_position() + fvector(32, 0);
	case UP:
		return get_relative_position() - fvector(0, 32);
	case DOWN:
		return get_relative_position() + fvector(0, 32);
	}
	return get_relative_position();
}

utility::error
entity::set_cycle_group(std::string name)
{
	set_cycle_animation(name + ":left",  entity::LEFT ).handle_error(); // The directional walk cycles are optional
	set_cycle_animation(name + ":right", entity::RIGHT).handle_error();
	set_cycle_animation(name + ":up",    entity::UP   ).handle_error();
	set_cycle_animation(name + ":down",  entity::DOWN ).handle_error();
	if (set_cycle_animation(name + ":default", entity::DEFAULT).handle_error())      // default is required though...
		return "Default animation/sprite is required for cycle group '" + name + "'";
	return 0;
}
