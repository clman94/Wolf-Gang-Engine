#ifndef RPG_PLAYER_CHARACTER
#define RPG_PLAYER_CHARACTER

#include <rpg/character_entity.hpp>
#include <rpg/controls.hpp>
#include <rpg/collision_system.hpp>

namespace rpg {

// The main player character_entity
class player_character :
	public character_entity
{
public:
	void clean();

	player_character();
	void set_locked(bool pLocked);
	bool is_locked();

	// Do movement with collision detection
	void movement(controls &pControls, collision_system& pCollision_system, float pDelta);

	// Get point in front of player
	engine::fvector get_activation_point(float pDistance = 0.6f);
	engine::frect   get_collision_box() const;

private:
	bool mLocked;
	void set_move_direction(engine::fvector pVec);
};

}
#endif // !RPG_PLAYER_CHARACTER
