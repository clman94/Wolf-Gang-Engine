#ifndef RPG_CHARACTER_ENTITY_HPP
#define RPG_CHARACTER_ENTITY_HPP

#include <rpg/sprite_entity.hpp>

namespace rpg {

// An sprite_entity that has a specific role as a character_entity.
// Provides walk cycles.
class character_entity :
	public sprite_entity
{
public:

	enum class cycle
	{
		def, // "default" is apparently not allowed in gcc....
		left,
		right,
		up,
		down,
		idle,
		idle_left,
		idle_right,
		idle_up,
		idle_down
	};

	enum class direction
	{
		other,
		left,
		right,
		up,
		down,
	};

	character_entity();
	void set_cycle_group(const std::string& name);
	void set_cycle(const std::string& name);
	void set_cycle(cycle type);

	void set_direction(direction pDirection);
	void set_direction(engine::fvector pVector);
	void set_direction_not_relative(engine::fvector pVector);
	direction get_direction();

	void set_idle(bool pIs_idle);
	bool is_idle();

	void  set_speed(float f);
	float get_speed();

private:
	std::string mCyclegroup;
	std::string mCycle;
	direction mDirection;
	bool mIs_idle;
	float mMove_speed;
};

}
#endif // !RPG_SPRITE_ENTITY_HPP
