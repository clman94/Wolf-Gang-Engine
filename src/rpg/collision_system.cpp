#include <rpg/collision_system.hpp>

using namespace rpg;

util::optional<engine::fvector> collision_system::get_door_entry(std::string pName)
{
	for (auto& i : mContainer.get_boxes())
	{
		if (i->get_type() == collision_box::type::door)
		{
			std::shared_ptr<const door> find = std::dynamic_pointer_cast<const door>(i);
			if (find->get_name() != pName)
				continue;

			const engine::frect& region = find->get_region()*32;
			return region.get_offset() + (region.get_size()*0.5f) + find->get_offset()*32;
		}
	}
	return{};
}

void collision_system::clean()
{
	mContainer.clean();
}

int collision_system::load_collision_boxes(tinyxml2::XMLElement* pEle)
{
	mContainer.load_xml(pEle);
	return 0;
}

void collision_system::setup_script_defined_triggers(const script_context & pContext)
{
	for (auto& i : pContext.get_wall_group_functions())
	{
		auto group = mContainer.get_group(i.group);
		if (!group)
		{
			util::warning("Group '" + i.group + "' does not exist");
			continue;
		}
		group->add_function(*i.function);
	}
}

void collision_system::load_script_interface(script_system & pScript)
{
	pScript.add_function("void _set_wall_group_enabled(const string&in, bool)", AS::asMETHOD(collision_system, script_set_wall_group_enabled), this);
	pScript.add_function("bool _get_wall_group_enabled(const string&in)", AS::asMETHOD(collision_system, script_get_wall_group_enabled), this);
}

collision_box_container& collision_system::get_container()
{
	return mContainer;
}

void collision_system::script_set_wall_group_enabled(const std::string& pName, bool pEnabled)
{
	auto group = mContainer.get_group(pName);
	if (!group)
	{
		util::warning("Enable to find wall group '" + pName + "'");
		return;
	}

	group->set_enabled(pEnabled);
}

bool collision_system::script_get_wall_group_enabled(const std::string & pName)
{
	auto group = mContainer.get_group(pName);
	if (!group)
	{
		util::warning("Enable to find wall group '" + pName + "'");
		return false;
	}

	return group->is_enabled();
}
