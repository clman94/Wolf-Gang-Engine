#include <rpg/player_character.hpp>

using namespace rpg;

void player_character::set_move_direction(engine::fvector pVec)
{
	if (std::abs(pVec.x) > std::abs(pVec.y))
	{
		if (pVec.x > 0)
			set_direction(direction::right);
		else
			set_direction(direction::left);
	}
	else
	{
		if (pVec.y > 0)
			set_direction(direction::down);
		else
			set_direction(direction::up);
	}
}

void player_character::clean()
{
	set_color({ 255, 255, 255, 255 });
	set_position({ 0, 0 });
	set_locked(false);
	set_cycle_group("default");
	set_cycle("default");
	set_dynamic_depth(true);
	set_visible(true);
	set_rotation(0);
}

player_character::player_character()
{
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
		set_direction(direction::left);
	}

	if (pControls.is_triggered(controls::control::right))
	{
		move.x += 1;
		set_direction(direction::right);
	}

	if (pControls.is_triggered(controls::control::up))
	{
		move.y -= 1;
		set_direction(direction::up);
	}

	if (pControls.is_triggered(controls::control::down))
	{
		move.y += 1;
		set_direction(direction::down);
	}

	// Check collision if requested to move
	if (move != engine::fvector(0, 0))
	{
		// Normalize so movement is consistant
		move.normalize();
		move *= get_speed()*pDelta;

		const engine::frect collision_box = get_collision_box();
		engine::frect collision_box_modified = collision_box;

		// Check if there is a tile blocking the x axis
		collision_box_modified.set_offset(collision_box.get_offset() + engine::fvector(move.x, 0));
		if (pCollision_system.get_container().first_collision(collision_box::type::wall, engine::scale(collision_box_modified, 1.f / 32)))
			move.x = 0; // Player is unable to move in the X directions

		// Check if there is a tile blocking the y axis
		collision_box_modified.set_offset(collision_box.get_offset() + engine::fvector(0, move.y));
		if (pCollision_system.get_container().first_collision(collision_box::type::wall, engine::scale(collision_box_modified, 1.f / 32)))
			move.y = 0; // Player is unable to move in the Y directions
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
	switch (get_direction())
	{
	case direction::other: return get_position();
	case direction::left:  return get_position() + engine::fvector(-pDistance, 0);
	case direction::right: return get_position() + engine::fvector(pDistance, 0);
	case direction::up:    return get_position() + engine::fvector(0, -pDistance);
	case direction::down:  return get_position() + engine::fvector(0, pDistance);
	}
	return{ 0, 0 };
}

engine::frect player_character::get_collision_box() const
{
	const engine::fvector collision_size(26, 15);
	const engine::fvector collision_offset
		= get_position()
		- engine::fvector(collision_size.x / 2, collision_size.y);
	return engine::frect(collision_offset, collision_size);
}
