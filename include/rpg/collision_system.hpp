#ifndef RPG_COLLISION_SYSTEM_HPP
#define RPG_COLLISION_SYSTEM_HPP

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

// A simple static collision system for world interactivity
class collision_system
{
public:
	util::optional_pointer<collision_box> wall_collision(const engine::frect& r);
	util::optional_pointer<door>          door_collision(const engine::fvector& pPosition);
	util::optional_pointer<trigger>       trigger_collision(const engine::fvector& pPosition);
	util::optional_pointer<trigger>       button_collision(const engine::fvector& pPosition);

	util::optional<engine::fvector> get_door_entry(std::string pName);

	void validate_all(flag_container& pFlags);

	void add_wall(engine::frect r);
	void add_trigger(trigger& t);
	void add_button(trigger& t);
	void clean();

	int load_collision_boxes(tinyxml2::XMLElement* pEle);

private:
	std::list<collision_box> mWalls;
	std::list<door> mDoors;
	std::list<trigger> mTriggers;
	std::list<trigger> mButtons;
};


}
#endif // !RPG_COLLISION_SYSTEM_HPP
