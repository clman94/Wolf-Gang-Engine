#include <rpg/collision_system.hpp>

using namespace rpg;

util::optional_pointer<collision_box> collision_system::wall_collision(const engine::frect& r)
{
	for (auto &i : mWalls)
		if (i.is_enabled() && i.get_region().is_intersect(r))
			return &i;
	return{};
}

util::optional_pointer<door> collision_system::door_collision(const engine::fvector& pPosition)
{
	for (auto &i : mDoors)
		if (i.is_enabled() && i.get_region().is_intersect(pPosition))
			return &i;
	return{};
}

util::optional_pointer<trigger> collision_system::trigger_collision(const engine::fvector& pPosition)
{
	for (auto &i : mTriggers)
		if (i.is_enabled() && i.get_region().is_intersect(pPosition))
			return &i;
	return{};
}

util::optional_pointer<trigger> collision_system::button_collision(const engine::fvector& pPosition)
{
	for (auto &i : mButtons)
		if (i.is_enabled() && i.get_region().is_intersect(pPosition))
			return &i;
	return{};
}

util::optional<engine::fvector> collision_system::get_door_entry(std::string pName)
{
	for (auto& i : mDoors)
	{
		if (i.name == pName)
		{
			engine::frect region = i.get_region();
			return region.get_offset() + (region.get_size()*0.5f) + i.offset;
		}
	}
	return{};
}


void collision_system::add_wall(engine::frect r)
{
	collision_box nw;
	nw.set_region(r);
	mWalls.push_back(nw);
}

void collision_system::add_trigger(trigger & t)
{
	mTriggers.push_back(t);
}

void collision_system::add_button(trigger & t)
{
	mButtons.push_back(t);
}

void collision_system::clean()
{
	mWalls.clear();
	mDoors.clear();
	mTriggers.clear();
	mButtons.clear();
}

int collision_system::load_collision_boxes(tinyxml2::XMLElement* pEle, const scene_loader& pScene_loader)
{
	assert(pEle != nullptr);

	for (auto i : pScene_loader.get_walls())
		mWalls.emplace_back(i*32);

	mWall_groups = pScene_loader.get_wall_groups();

	auto ele_item = pEle->FirstChildElement("door");
	while (ele_item)
	{
		door nd;
		nd.load_xml(ele_item);
		nd.name        = util::safe_string(ele_item->Attribute("name"));
		nd.destination = util::safe_string(ele_item->Attribute("destination"));
		nd.scene_path  = util::safe_string(ele_item->Attribute("scene"));
		nd.offset.x    = ele_item->FloatAttribute("offsetx") * 32;
		nd.offset.y    = ele_item->FloatAttribute("offsety") * 32;
		mDoors.push_back(nd);

		ele_item = ele_item->NextSiblingElement();
	}
	return 0;
}

void collision_system::setup_script_defined_triggers(const script_context & pContext)
{
	mTriggers = pContext.get_script_defined_triggers();
	mButtons = pContext.get_script_defined_buttons();
}

void collision_system::load_script_interface(script_system & pScript)
{
	pScript.add_function("void set_wall_group_enabled(const string&in, bool)", AS::asMETHOD(collision_system, script_set_wall_group_enabled), this);
}

void collision_system::script_set_wall_group_enabled(const std::string& pName, bool pEnabled)
{
	if (mWall_groups.find(pName) == mWall_groups.end())
	{
		util::error("Enable to find wall group '" + pName + "'");
		return;
	}
	for (auto i : mWall_groups[pName])
		mWalls[i].set_enable(pEnabled);
}
