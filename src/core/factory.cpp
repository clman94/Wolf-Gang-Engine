#include <wge/core/factory.hpp>

namespace wge::core
{

component* factory::create_component(const component_type& pType, component_manager& pManager) const
{
	auto iter = mComponent_factories.find(pType);
	if (iter == mComponent_factories.end())
		return nullptr;
	return iter->second(pManager);
}

system::uptr factory::create_system(int pType, layer& pLayer) const
{
	auto iter = mSystem_factories.find(pType);
	if (iter == mSystem_factories.end())
		return nullptr;
	return iter->second(pLayer);
}

} // namespace wge::core
