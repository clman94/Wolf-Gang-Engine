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

			return find->calculate_player_position();
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

void collision_system::setup_script_defined_triggers(const scene_script_context & pContext)
{
	for (auto& i : pContext.get_wall_group_functions())
	{
		auto group = mContainer.get_group(i.group);
		if (!group)
		{
			util::warning("Group '" + i.group + "' does not exist");
			continue;
		}
		group->add_function(i.function);
	}
}

void collision_system::load_script_interface(script_system & pScript)
{
	mScript = &pScript;
	register_collision_type(pScript);
	pScript.set_namespace("collision");
	pScript.add_function("box _create_box(int)", AS::asMETHOD(collision_system, script_create_box), this);
	pScript.add_function("void set_position(box&in, const vec&in)", AS::asMETHOD(collision_system, script_set_box_position), this);
	pScript.add_function("void set_size(box&in, const vec&in)", AS::asMETHOD(collision_system, script_set_box_size), this);
	pScript.add_function("void set_group(box&in, const string&in)", AS::asMETHOD(collision_system, script_set_box_group), this);
	pScript.reset_namespace();

	pScript.add_function("void _set_wall_group_enabled(const string&in, bool)", AS::asMETHOD(collision_system, script_set_wall_group_enabled), this);
	pScript.add_function("bool _get_wall_group_enabled(const string&in)", AS::asMETHOD(collision_system, script_get_wall_group_enabled), this);
	pScript.add_function("void _bind_box_function(coroutine@+, dictionary @+)", AS::asMETHOD(collision_system, script_bind_group_function), this);

}

collision_box_container& collision_system::get_container()
{
	return mContainer;
}

void collision_system::register_collision_type(script_system& pScript)
{
	pScript.set_namespace("collision");
	auto& engine = pScript.get_engine();
	engine.RegisterObjectType("box", sizeof(std::shared_ptr<collision_box>), AS::asOBJ_VALUE | AS::asGetTypeTraits<std::shared_ptr<collision_box>>());
	
	// Constructors and deconstructors
	engine.RegisterObjectBehaviour("box", AS::asBEHAVE_CONSTRUCT, "void f()"
		, AS::asFUNCTION(script_system::script_default_constructor<std::shared_ptr<collision_box>>)
		, AS::asCALL_CDECL_OBJLAST);
	engine.RegisterObjectBehaviour("box", AS::asBEHAVE_DESTRUCT, "void f()"
		, AS::asFUNCTION(script_system::script_default_deconstructor<std::shared_ptr<collision_box>>)
		, AS::asCALL_CDECL_OBJLAST);

	// Assignments
	engine.RegisterObjectMethod("box", "box& opAssign(const box &in)"
		, AS::asMETHODPR(std::shared_ptr<collision_box>, operator=, (const std::shared_ptr<collision_box>&), std::shared_ptr<collision_box>&)
		, AS::asCALL_THISCALL);

	pScript.reset_namespace();
}

void collision_system::script_create_wall_group(const std::string & pName)
{
	mContainer.create_group(pName);
}

void collision_system::script_set_wall_group_enabled(const std::string& pName, bool pEnabled)
{
	auto group = mContainer.get_group(pName);
	if (!group)
	{
		util::warning("Unable to find wall group '" + pName + "'");
		return;
	}

	group->set_enabled(pEnabled);
}

bool collision_system::script_get_wall_group_enabled(const std::string & pName)
{
	auto group = mContainer.get_group(pName);
	if (!group)
	{
		util::warning("Unable to find wall group '" + pName + "'");
		return false;
	}

	return group->is_enabled();
}

void collision_system::script_bind_group_function(const std::string & pName, AS::asIScriptFunction * pFunc)
{
	util::error("Implementation Incomplete");
	return;

	auto group = mContainer.get_group(pName);
	if (!group)
	{
		util::warning("Unable to find wall group '" + pName + "'");
		return;
	}

	std::shared_ptr<script_function> func(new script_function);
	func->set_function(pFunc);
	func->set_script_system(*mScript);
	group->add_function(func);
}

std::shared_ptr<collision_box> collision_system::script_create_box(collision_box::type pType)
{
	return mContainer.add_collision_box(pType);
}

void collision_system::script_set_box_group(std::shared_ptr<collision_box>& pBox, const std::string & pName)
{
	auto group = mContainer.get_group(pName);
	if (!group)
	{
		util::warning("Unable to find wall group '" + pName + "'");
		return;
	}
	pBox->set_wall_group(group);
}

void collision_system::script_set_box_position(std::shared_ptr<collision_box>& pBox, const engine::fvector & pPosition)
{
	if (!pBox)
	{
		util::error("Invalid box reference");
		return;
	}
	auto region = pBox->get_region();
	region.set_offset(pPosition);
	pBox->set_region(region);
}

void collision_system::script_set_box_size(std::shared_ptr<collision_box>& pBox, const engine::fvector & pSize)
{
	if (!pBox)
	{
		util::error("Invalid box reference");
		return;
	}
	auto region = pBox->get_region();
	region.set_size(pSize);
	pBox->set_region(region);
}
