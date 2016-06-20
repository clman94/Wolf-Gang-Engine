#include "rpg_entity.hpp"
using namespace rpg;

entity::animation*
entity::find_animation(std::string name, std::list<animation>& list)
{
	for (auto& i : list)
	{
		if (i.name == name)
			return &i;
	}
	return nullptr;
}

entity::entity()
{
	for (int i = 0; i < 5; i++)
	{
		world_animation[i] = nullptr;
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

void
entity::set_cycle_animation(std::string _name, cycle_type cycle)
{
	auto a = find_animation(_name, world);
	if (!a)
	{
		std::cout << "Error: entity animation '" << _name 
			<< "' in entity '" << name <<"' does not exist.\n";
		return;
	}
	a->node.set_relative_position(a->node.get_size() * -engine::fvector(0.5, 1)); // Anchor at bottom
	world_animation[cycle] = a;
}

int
entity::draw(engine::renderer &_r)
{
	if (world_animation[c_cycle])
		world_animation[c_cycle]->node.draw(_r);
	return 0;
}

void
entity::move_left(float delta)
{
	set_relative_position(get_relative_position() + engine::fvector(-delta));
}

void
entity::move_right(float delta)
{
	set_relative_position(get_relative_position() + engine::fvector(delta));
}

void
entity::move_up(float delta)
{
	set_relative_position(get_relative_position() + engine::fvector(0, -delta));
}

void
entity::move_down(float delta)
{
	set_relative_position(get_relative_position() + engine::fvector(0, delta));
}

void
entity::set_cycle(int cycle)
{
	c_cycle = cycle;
	if (world_animation[cycle])
		c_anim = world_animation[cycle];
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
