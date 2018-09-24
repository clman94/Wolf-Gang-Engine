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

context* component::get_context() const
{
	assert(mObject);
	return mObject->get_context();
}

asset_manager* component::get_asset_manager() const
{
	return get_context()->get_system<asset_manager>();
}
