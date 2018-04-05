#include <rpg/collision_system.hpp>
#include <engine/logger.hpp>

using namespace rpg;

std::shared_ptr<const door> collision_system::get_door_entry(std::string pName)
{
	for (auto& i : mContainer.get_boxes())
	{
		if (i->get_type() == collision_box::type::door)
		{
			std::shared_ptr<const door> find = std::dynamic_pointer_cast<const door>(i);
			if (find->get_name() != pName)
				continue;

			return find;
		}
	}
	return{};
}

void collision_system::clear()
{
	mContainer.clear();
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
			logger::warning("Group '" + i.group + "' does not exist");
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

	pScript.add_enum("type");
	pScript.add_enum_class_value("type", "wall", collision_box::type::wall);
	pScript.add_enum_class_value("type", "trigger", collision_box::type::trigger);
	pScript.add_enum_class_value("type", "button", collision_box::type::button);
	pScript.add_enum_class_value("type", "door", collision_box::type::door);

	pScript.add_function("_create_box", &collision_system::script_create_box, this);
	pScript.add_function("set_position", &collision_system::script_set_box_position, this);
	pScript.add_function("set_size", &collision_system::script_set_box_size, this);
	pScript.add_function("set_group", &collision_system::script_set_box_group, this);
	pScript.add_function("is_colliding", &collision_system::script_collision_is_colliding_rect, this);
	pScript.add_function("is_colliding", &collision_system::script_collision_is_colliding_vec, this);
	pScript.add_function("first_collision", &collision_system::script_collision_first_collision_rect, this);
	pScript.add_function("first_collision", &collision_system::script_collision_first_collision_vec, this);
	pScript.add_function("first_collision", &collision_system::script_collision_first_collision_type_rect, this);
	pScript.add_function("first_collision", &collision_system::script_collision_first_collision_type_vec, this);
	pScript.add_function("get_group", &collision_system::script_get_box_group, this);
	pScript.add_function("get_dest_door", &collision_system::script_collision_get_door_dest, this);
	pScript.add_function("get_dest_scene", &collision_system::script_collision_get_door_scene, this);
	pScript.add_function("activate_triggers", &collision_system::script_activate_triggers, this);
	pScript.reset_namespace();

	pScript.set_namespace("group");
	pScript.add_function("enable", &collision_system::script_set_wall_group_enabled, this);
	pScript.add_function("is_enabled", &collision_system::script_get_wall_group_enabled, this);
	pScript.add_function("call_bindings", &collision_system::script_call_group_functions, this);
	pScript.reset_namespace();


	//pScript.add_function("void _bind_box_function(coroutine@+, dictionary @+)", AS::asMETHOD(collision_system, script_bind_group_function), this);
}

collision_box_container& collision_system::get_container()
{
	return mContainer;
}

void collision_system::register_collision_type(script_system& pScript)
{
	pScript.set_namespace("collision");

	pScript.add_object<collision_box::ptr>("box");
	pScript.add_method<collision_box::ptr, collision_box::ptr&, const collision_box::ptr&>("box", operator_method::assign, &collision_box::ptr::operator=);
	pScript.add_method<collision_box::ptr, bool>("box", operator_method::impl_conv, &collision_box::ptr::operator bool);

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
		logger::warning("Unable to find wall group '" + pName + "'");
		return;
	}

	group->set_enabled(pEnabled);
}

bool collision_system::script_get_wall_group_enabled(const std::string & pName)
{
	auto group = mContainer.get_group(pName);
	if (!group)
	{
		logger::warning("Unable to find wall group '" + pName + "'");
		return false;
	}

	return group->is_enabled();
}

void collision_system::script_bind_group_function(const std::string & pName, AS::asIScriptFunction * pFunc)
{
	logger::error("Implementation Incomplete");
	return;

	auto group = mContainer.get_group(pName);
	if (!group)
	{
		logger::warning("Unable to find wall group '" + pName + "'");
		return;
	}

	std::shared_ptr<script_function> func(new script_function);
	func->mFunction = pFunc;
	func->mScript_system = mScript;
	group->add_function(func);
}

void collision_system::script_call_group_functions(const std::string & pName)
{
	auto group = mContainer.get_group(pName);
	if (!group)
	{
		logger::print(*mScript, logger::level::error, "Unable to find wall group '" + pName + "'");
		return;
	}
	group->call_function();
}

bool collision_system::script_collision_is_colliding_rect(const engine::frect & pRect) const
{
	return mContainer.first_collision(collision_box::type::wall, pRect) != nullptr;
}

bool collision_system::script_collision_is_colliding_vec(const engine::fvector & pVec) const
{
	return mContainer.first_collision(collision_box::type::wall, pVec) != nullptr;
}

std::shared_ptr<collision_box> collision_system::script_collision_first_collision_rect(const engine::frect & pRect) const
{
	return mContainer.first_collision(pRect);
}

std::shared_ptr<collision_box> collision_system::script_collision_first_collision_vec(const engine::fvector & pVec) const
{
	return mContainer.first_collision(pVec);
}

std::shared_ptr<collision_box> collision_system::script_collision_first_collision_type_rect(collision_box::type pType, const engine::frect & pRect) const
{
	return mContainer.first_collision(pType, pRect);
}

std::shared_ptr<collision_box> collision_system::script_collision_first_collision_type_vec(collision_box::type pType, const engine::fvector & pVec) const
{
	return mContainer.first_collision(pType, pVec);
}

bool collision_system::script_activate_triggers(const engine::frect & pRect) const
{
	auto triggers = mContainer.collision(collision_box::type::trigger, pRect);
	for (auto& i : triggers)
		std::dynamic_pointer_cast<trigger>(i)->call_function();
	return !triggers.empty();
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
		logger::print(*mScript, logger::level::error, "Unable to find wall group '" + pName + "'");
		return;
	}
	pBox->set_wall_group(group);
}

void collision_system::script_set_box_position(std::shared_ptr<collision_box>& pBox, const engine::fvector & pPosition)
{
	if (!pBox)
	{
		logger::print(*mScript, logger::level::error, "Invalid Box handle");
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
		logger::print(*mScript, logger::level::error, "Invalid Box handle");
		return;
	}
	auto region = pBox->get_region();
	region.set_size(pSize);
	pBox->set_region(region);
}

std::string collision_system::script_get_box_group(std::shared_ptr<collision_box>& pBox)
{
	if (!pBox)
	{
		logger::print(*mScript, logger::level::error, "Invalid Box handle");
		return{};
	}
	if (!pBox->get_wall_group())
		return{};
	return pBox->get_wall_group()->get_name();
}

collision_box::type collision_system::script_collision_get_type(std::shared_ptr<collision_box>& pBox) const
{
	if (!pBox)
	{
		logger::print(*mScript, logger::level::error, "Invalid Box handle");
		return collision_box::type::wall;
	}
	return pBox->get_type();
}

std::string collision_system::script_collision_get_door_dest(std::shared_ptr<collision_box>& pBox) const
{
	if (!pBox)
	{
		logger::print(*mScript, logger::level::error, "Invalid Box handle");
		return{};
	}
	auto box_door = std::dynamic_pointer_cast<door>(pBox);
	if (!box_door)
	{
		logger::print(*mScript, logger::level::error, "This box is not a door");
		return{};
	}
	return box_door->get_destination();
}

std::string collision_system::script_collision_get_door_scene(std::shared_ptr<collision_box>& pBox) const
{
	if (!pBox)
	{
		logger::print(*mScript, logger::level::error, "Invalid Box handle");
		return{};
	}
	auto box_door = std::dynamic_pointer_cast<door>(pBox);
	if (!box_door)
	{
		logger::print(*mScript, logger::level::error, "This box is not a door");
		return{};
	}
	return box_door->get_scene();
}

