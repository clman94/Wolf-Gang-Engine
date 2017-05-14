#include <rpg/character_entity.hpp>
#include <rpg/rpg_config.hpp>

using namespace rpg;

character_entity::character_entity()
{
	mCyclegroup = "default";
	mMove_speed = 3.f;
	mIs_idle = false;
}

void
character_entity::set_cycle_group(const std::string& name)
{
	mCyclegroup = name;
	mSprite.set_animation(mCyclegroup + ":" + mCycle, mSprite.is_playing());
}

void
character_entity::set_cycle(const std::string& name)
{
	if (mCycle != name)
	{
		mSprite.set_animation(mCyclegroup + ":" + name, mSprite.is_playing());
		mCycle = name;
	}
}

void
character_entity::set_cycle(cycle type)
{
	switch (type)
	{
	case cycle::def:        set_cycle("default");    break;
	case cycle::left:       set_cycle("left");       break;
	case cycle::right:      set_cycle("right");      break;
	case cycle::up:         set_cycle("up");         break;
	case cycle::down:       set_cycle("down");       break;
	case cycle::idle:       set_cycle("idle");       break;
	case cycle::idle_left:  set_cycle("idle_left");  break;
	case cycle::idle_right: set_cycle("idle_right"); break;
	case cycle::idle_up:    set_cycle("idle_up");    break;
	case cycle::idle_down:  set_cycle("idle_down");  break;
	}
}

void character_entity::set_direction(direction pDirection)
{
	mDirection = pDirection;

	// Converting from direction to cycle type
	/// TODO: Too messy, clean up
	int offset = static_cast<int>(pDirection) - static_cast<int>(direction::left);
	if (!mIs_idle)
		set_cycle(static_cast<cycle>(static_cast<int>(cycle::left) + offset));
	else
		set_cycle(static_cast<cycle>(static_cast<int>(cycle::idle_left) + offset));
}

void character_entity::set_direction(engine::fvector pVector)
{
	set_direction_not_relative(pVector - get_position());
}

void character_entity::set_direction_not_relative(engine::fvector pVector)
{
	if (std::abs(pVector.x) > std::abs(pVector.y))
	{
		if (pVector.x > 0)
			set_direction(direction::right);
		else
			set_direction(direction::left);
	}
	else
	{
		if (pVector.y > 0)
			set_direction(direction::down);
		else
			set_direction(direction::up);
	}
}

character_entity::direction character_entity::get_direction()
{
	return mDirection;
}

void character_entity::set_idle(bool pIs_idle)
{
	mIs_idle = pIs_idle;
}

bool character_entity::is_idle()
{
	return mIs_idle;
}

void
character_entity::set_speed(float pSpeed)
{
	mMove_speed = pSpeed;
}

float
character_entity::get_speed()
{
	return mMove_speed;
}