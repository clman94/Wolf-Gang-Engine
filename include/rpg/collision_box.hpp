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
	collision_box(engine::frect pRect);
	bool is_enabled() const;
	void set_enable(bool pEnabled);
	void load_xml(tinyxml2::XMLElement* e);
	engine::frect get_region() const;
	void set_region(engine::frect pRegion);

protected:
	bool mEnabled;
	engine::frect mRegion;
};

// A collisionbox that is activated once the player has walked over it.
struct trigger : public collision_box
{
public:
	trigger();
	void set_function(script_function& pFunction);
	bool call_function();

	void parse_function_metadata(const std::string& pMetadata);

private:
	script_function* mFunction;
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