#include <rpg/player_character.hpp>

using namespace rpg;

void player_character::reset()
{
	mSprite.set_color({ 1, 1, 1, 1 });
	mSprite.set_rotation(0);

	set_position({ 0, 0 });
	set_locked(false);
	set_cycle_group("default");
	set_cycle("default");
	set_dynamic_depth(true);
	set_visible(false);
	set_z(0);
}

player_character::player_character()
{
	mIs_walking = false;
}

void player_character::set_locked(bool pLocked)
{
	mLocked = pLocked;
}

bool player_character::is_locked()
{
	return mLocked;
}

void player_character::movement(engine::controls& pControls, collision_system& pCollision_system, float pDelta)
{
	if (mLocked)
	{
		if (mIs_walking) // Reset animation
		{
			mIs_walking = false;
			mSprite.stop();
		}
		return;
	}

	engine::fvector move(0, 0);
	if (pControls.is_triggered("left"))  move.x -= 1;
	if (pControls.is_triggered("right")) move.x += 1;
	if (pControls.is_triggered("up"))    move.y -= 1;
	if (pControls.is_triggered("down"))  move.y += 1;

	// Check collision if requested to move
	if (move != engine::fvector(0, 0))
	{
		// Normalize so movement is consistant
		move.normalize();
		move *= get_speed()*pDelta;

		const engine::frect collision_box = get_collision_box();
		engine::frect collision_box_modified = collision_box;

		engine::fvector modified_move = move;
		bool has_collision = false;

		// Check if there is a tile blocking the x axis
		collision_box_modified.set_offset(collision_box.get_offset() + engine::fvector(move.x, 0));
		if (pCollision_system.get_container().first_collision(collision_box::type::wall, collision_box_modified))
		{
			has_collision = true;
			modified_move.x = 0; // Player is unable to move in the X directions
		}

		// Check if there is a tile blocking the y axis
		collision_box_modified.set_offset(collision_box.get_offset() + engine::fvector(0, move.y));
		if (pCollision_system.get_container().first_collision(collision_box::type::wall, collision_box_modified))
		{
			has_collision = true;
			modified_move.y = 0; // Player is unable to move in the Y directions
		}

		if (has_collision)
		{
			if (modified_move == engine::fvector(0, 0))
				set_direction(vector_direction(move)); // Can't move but can still change direction
			else
				set_direction(vector_direction(modified_move)); // Make sure the player is in the direction it is moving
		}
		else
			walking_direction(move); // Special direction thing

		move = modified_move;
	}

	// Move if possible
	if (move != engine::fvector(0, 0))
	{
		set_position(get_position() + move);
		mIs_walking = true;
		mSprite.tick();
	}
	else if (mIs_walking) // Reset animation
	{
		mIs_walking = false;
		mSprite.stop();
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
	const engine::fvector collision_size = engine::fvector(mSprite.get_size().x, mSprite.get_size().y / 3) / get_unit(); // get_size returns pixels; convert to tile grid
	const engine::fvector collision_offset
		= get_position()
		- engine::fvector(collision_size.x / 2, collision_size.y);
	return engine::frect(collision_offset, collision_size);
}

void player_character::walking_direction(engine::fvector pMove)
{
	if (!mIs_walking) // First click sets the direction
		set_direction(vector_direction(pMove));
	else
	{
		// Player direction locked left and right
		if ((get_direction() == direction::left
			|| get_direction() == direction::right)
			|| (pMove.x != 0 && pMove.y == 0))
			set_direction(vector_direction(engine::fvector::x_only(pMove)));

		// Player direction locked up and down
		if ((get_direction() == direction::up
			|| get_direction() == direction::down)
			|| (pMove.x == 0 && pMove.y != 0))
			set_direction(vector_direction(engine::fvector::y_only(pMove)));
	}
}
