#include <wge/core/component.hpp>
#include <wge/core/game_object.hpp>
#include <wge/core/context.hpp>
#include <wge/core/asset_manager.hpp>

namespace wge::core
{

json component::serialize(serialize_type pType) const
{
	json result;
	result["name"] = mName;
	result["type"] = get_component_id();
	result["data"] = on_serialize(pType);
	return result;
}

void component::deserialize(const game_object& pObj, const json& pJson)
{
	mName = pJson["name"];
	on_deserialize(pObj, pJson["data"]);
}

const void component::set_name(const std::string & pName) noexcept
{
	mName = pName;
}

const std::string& component::get_name() const noexcept
{
	return mName;
}

object_id component::get_object_id() const noexcept
{
	return mObject_id;
}

void component::set_object(const game_object & pObj) noexcept
{
	mObject_id = pObj.get_instance_id();
}

component_id component::get_instance_id() const noexcept
{
	return mInstance_id;
}

} // namespace wge::core
