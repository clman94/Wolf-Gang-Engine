#include <wge/core/component.hpp>
#include <wge/core/object_node.hpp>
#include <wge/core/context.hpp>
#include <wge/core/asset_manager.hpp>

using namespace wge;
using namespace wge::core;

const void component::set_name(const std::string & pName)
{
	mName = pName;
}

const std::string& component::get_name() const
{
	return mName;
}

void component::set_object(const game_object & pObj)
{
	mObject_id = pObj.get_instance_id();
}
