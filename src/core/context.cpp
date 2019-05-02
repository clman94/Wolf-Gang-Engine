#include <wge/core/context.hpp>
#include <wge/util/ptr.hpp>
#include <wge/core/engine.hpp>

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

layer::ptr context::get_layer(std::size_t pIndex) const
{
	if (pIndex >= mLayers.size())
		return{};
	return mLayers[pIndex];
}

layer::ptr context::create_unhandled_layer()
{
	return layer::create(*this);
}

layer::ptr context::add_layer()
{
	return mLayers.emplace_back(layer::create(*this));
}

layer::ptr context::add_layer(const std::string& pName)
{
	auto l = add_layer();
	l->set_name(pName);
	return l;
}

layer::ptr context::add_layer(const std::string& pName, std::size_t pInsert)
{
	mLayers.insert(mLayers.begin() + pInsert, layer::create(*this));
	return mLayers[pInsert];
}

void context::remove_layer(const layer* pPtr)
{
	for (std::size_t i = 0; i < mLayers.size(); i++)
	{
		if (util::to_address(mLayers[i]) == pPtr)
		{
			mLayers.erase(mLayers.begin() + i);
			return;
		}
	}
}

void context::remove_layer(const layer::ptr& pPtr)
{
	remove_layer(util::to_address(pPtr));
}

const context::layers& context::get_layer_container() const noexcept
{
	return mLayers;
}

asset_manager& context::get_asset_manager() noexcept
{
	return mEngine.get_asset_manager();
}

const factory& context::get_factory() const noexcept
{
	return mEngine.get_factory();
}

void context::preupdate(float pDelta)
{
	for (auto& i : mLayers)
		i->preupdate(pDelta);
}

void context::update(float pDelta)
{
	for (auto& i : mLayers)
		i->update(pDelta);
}

void context::postupdate(float pDelta)
{
	for (auto& i : mLayers)
		i->postupdate(pDelta);
}

void context::clear()
{
	mLayers.clear();
}

} // namespace wge::core
