#include <wge/core/factory.hpp>

namespace wge::core
{

util::copyable_ptr<system> factory::create_system(int pType, layer& pLayer) const
{
	auto iter = mSystem_factories.find(pType);
	if (iter == mSystem_factories.end())
		return nullptr;
	return iter->second(pLayer);
}

} // namespace wge::core
