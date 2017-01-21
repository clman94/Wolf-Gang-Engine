#ifndef RPG_COLLISION_SYSTEM_HPP
#define RPG_COLLISION_SYSTEM_HPP

#include "../../tinyxml2/tinyxml2.h"

#include <string>

#include <engine/rect.hpp>
#include <engine/utility.hpp>

#include <rpg/script_context.hpp>
#include <rpg/flag_container.hpp>
#include <rpg/collision_box.hpp>
#include <rpg/scene_loader.hpp>

namespace rpg {

// A simple static collision system for world interactivity
class collision_system
{
public:
	util::optional_pointer<collision_box> wall_collision(const engine::frect& r);
	util::optional_pointer<door>          door_collision(const engine::fvector& pPosition);
	util::optional_pointer<trigger>       trigger_collision(const engine::fvector& pPosition);
	util::optional_pointer<trigger>       button_collision(const engine::fvector& pPosition);

	util::optional<engine::fvector> get_door_entry(std::string pName);

	void add_wall(engine::frect r);
	void add_trigger(trigger& t);
	void add_button(trigger& t);
	void clean();

	int load_collision_boxes(tinyxml2::XMLElement* pEle, const scene_loader& pScene_loader);
	void setup_script_defined_triggers(const script_context& pContext);
	void load_script_interface(script_system& pScript);

private:
	wall_groups_t mWall_groups;
	std::vector<collision_box> mWalls;
	std::vector<door> mDoors;
	std::vector<trigger> mTriggers;
	std::vector<trigger> mButtons;

	void script_set_wall_group_enabled(const std::string& pName, bool pEnabled);
};


}
#endif // !RPG_COLLISION_SYSTEM_HPP
