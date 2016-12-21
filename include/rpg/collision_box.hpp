#ifndef RPG_COLLISION_BOX_HPP
#define RPG_COLLISION_BOX_HPP

#include "../../tinyxml2/tinyxml2.h"

#include <string>

#include <engine/rect.hpp>
#include <engine/utility.hpp>

#include <rpg/script_function.hpp>
#include <rpg/flag_container.hpp>

namespace rpg {

// A basic collision box
struct collision_box
{
public:

	collision_box();
	bool is_valid();
	void validate(flag_container & pFlags);
	void load_xml(tinyxml2::XMLElement* e);
	engine::frect get_region();
	void set_region(engine::frect pRegion);

protected:
	std::string mInvalid_on_flag;
	std::string mSpawn_flag;
	bool valid;
	engine::frect mRegion;
};

// A collisionbox that is activated once the player has walked over it.
struct trigger : public collision_box
{
public:
	script_function& get_function();
	void parse_function_metadata(const std::string& pMetadata);

private:
	script_function mFunc;
};

struct door : public collision_box
{
	std::string name;
	std::string scene_path;
	std::string destination;
	engine::fvector offset;
};

}

#endif // !RPG_COLLISION_BOX_HPP