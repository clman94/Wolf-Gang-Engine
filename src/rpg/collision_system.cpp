#include <rpg/collision_system.hpp>

using namespace rpg;

util::optional_pointer<collision_box> collision_system::wall_collision(const engine::frect& r)
{
	for (auto &i : mWalls)
		if (i.is_valid() && i.get_region().is_intersect(r))
			return &i;
	return{};
}

util::optional_pointer<door> collision_system::door_collision(const engine::fvector& pPosition)
{
	for (auto &i : mDoors)
		if (i.is_valid() && i.get_region().is_intersect(pPosition))
			return &i;
	return{};
}

util::optional_pointer<trigger> collision_system::trigger_collision(const engine::fvector& pPosition)
{
	for (auto &i : mTriggers)
		if (i.is_valid() && i.get_region().is_intersect(pPosition))
			return &i;
	return{};
}

util::optional_pointer<trigger> collision_system::button_collision(const engine::fvector& pPosition)
{
	for (auto &i : mButtons)
		if (i.is_valid() && i.get_region().is_intersect(pPosition))
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

void collision_system::validate_all(flag_container& pFlags)
{
	for (auto &i : mWalls)    i.validate(pFlags);
	for (auto &i : mDoors)    i.validate(pFlags);
	for (auto &i : mTriggers) i.validate(pFlags);
	for (auto &i : mButtons)  i.validate(pFlags);
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

int collision_system::load_collision_boxes(tinyxml2::XMLElement* pEle)
{
	assert(pEle != nullptr);

	auto ele_item = pEle->FirstChildElement();
	while (ele_item)
	{
		std::string box_type = util::safe_string(ele_item->Name());

		if (box_type == "wall")
		{
			collision_box nw;
			nw.load_xml(ele_item);
			mWalls.emplace_back(nw);
		}

		if (box_type == "door")
		{
			door nd;
			nd.load_xml(ele_item);
			nd.name = util::safe_string(ele_item->Attribute("name"));
			nd.destination = util::safe_string(ele_item->Attribute("destination"));
			nd.scene_path = util::safe_string(ele_item->Attribute("scene"));
			nd.offset.x = ele_item->FloatAttribute("offsetx") * 32;
			nd.offset.y = ele_item->FloatAttribute("offsety") * 32;
			mDoors.push_back(nd);
		}

		ele_item = ele_item->NextSiblingElement();
	}
	return 0;
}