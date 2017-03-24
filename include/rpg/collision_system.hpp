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
	util::optional<engine::fvector> get_door_entry(std::string pName);

	void clean();

	int load_collision_boxes(tinyxml2::XMLElement* pEle);
	void setup_script_defined_triggers(const scene_script_context& pContext);
	void load_script_interface(script_system& pScript);

	collision_box_container& get_container();

private:

	collision_box_container mContainer;

	void script_set_wall_group_enabled(const std::string& pName, bool pEnabled);
	bool script_get_wall_group_enabled(const std::string& pName);
};

}
#endif // !RPG_COLLISION_SYSTEM_HPP
