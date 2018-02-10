#ifndef RPG_COLLISION_SYSTEM_HPP
#define RPG_COLLISION_SYSTEM_HPP

#include "../../3rdparty/tinyxml2/tinyxml2.h"

#include <string>

#include <engine/rect.hpp>
#include <engine/utility.hpp>

#include <rpg/script_context.hpp>
#include <rpg/flag_container.hpp>
#include <rpg/collision_box.hpp>
#include <rpg/scene_loader.hpp>

namespace util {
template<>
struct AS_type_to_string<std::shared_ptr<rpg::collision_box>> :
	AS_type_to_string_base
{
	AS_type_to_string()
	{
		mName = "box";
	}
};

template<>
struct AS_type_to_string<rpg::collision_box::type> :
	AS_type_to_string_base
{
	AS_type_to_string()
	{
		mName = "int";
	}
};


}

namespace rpg {

// A simple static collision system for world interactivity
class collision_system
{
public:
	std::shared_ptr<const door> get_door_entry(std::string pName);

	void clean();

	int load_collision_boxes(tinyxml2::XMLElement* pEle);
	void setup_script_defined_triggers(const scene_script_context& pContext);
	void load_script_interface(script_system& pScript);

	collision_box_container& get_container();

private:
	util::optional_pointer<script_system> mScript;

	collision_box_container mContainer;

	void register_collision_type(script_system& pScript);

	void script_create_wall_group(const std::string& pName);
	void script_set_wall_group_enabled(const std::string& pName, bool pEnabled);
	bool script_get_wall_group_enabled(const std::string& pName);
	void script_bind_group_function(const std::string& pName, AS::asIScriptFunction* func);

	std::shared_ptr<collision_box> script_create_box(collision_box::type pType);
	void script_set_box_group(std::shared_ptr<collision_box>& pBox, const std::string& pName);
	void script_set_box_position(std::shared_ptr<collision_box>& pBox, const engine::fvector& pPosition);
	void script_set_box_size(std::shared_ptr<collision_box>& pBox, const engine::fvector& pSize);

};

}
#endif // !RPG_COLLISION_SYSTEM_HPP
