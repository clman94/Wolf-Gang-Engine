#include <wge/core/scene.hpp>
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

layer* scene::get_layer(std::size_t pIndex) const
{
	if (pIndex >= mLayers.size())
		return{};
	return mLayers[pIndex].get();
}

layer::uptr scene::create_freestanding_layer()
{
	return layer::create(*this);
}

layer* scene::add_layer()
{
	return mLayers.emplace_back(layer::create(*this)).get();
}

layer* scene::add_layer(const std::string& pName)
{
	auto l = add_layer();
	l->set_name(pName);
	return l;
}

layer* scene::add_layer(const std::string& pName, std::size_t pInsert)
{
	return mLayers.insert(mLayers.begin() + pInsert, layer::create(*this))->get();
}

layer* scene::add_layer(layer::uptr& pPtr)
{
	return mLayers.emplace_back(std::move(pPtr)).get();
}

layer::uptr scene::release_layer(const layer& pLayer)
{
	for (std::size_t i = 0; i < mLayers.size(); i++)
	{
		if (mLayers[i].get() == &pLayer)
		{
			auto temp = std::move(mLayers[i]);
			mLayers.erase(mLayers.begin() + i);
			return temp;
		}
	}
	return{};
}


bool scene::remove_layer(const layer& pLayer)
{
	for (std::size_t i = 0; i < mLayers.size(); i++)
	{
		if (mLayers[i].get() == &pLayer)
		{
			mLayers.erase(mLayers.begin() + i);
			return true;
		}
	}
	return false;
}

const scene::layers& scene::get_layer_container() const noexcept
{
	return mLayers;
}

engine& scene::get_engine() const noexcept
{
	assert(mEngine);
	return *mEngine;
}

asset_manager& scene::get_asset_manager() noexcept
{
	return mEngine->get_asset_manager();
}

const factory& scene::get_factory() const noexcept
{
	return mEngine->get_factory();
}

void scene::preupdate(float pDelta)
{
	for (auto& i : mLayers)
		i->preupdate(pDelta);
}

void scene::update(float pDelta)
{
	for (auto& i : mLayers)
		i->update(pDelta);
}

void scene::postupdate(float pDelta)
{
	for (auto& i : mLayers)
		i->postupdate(pDelta);
}

void scene::clear()
{
	mLayers.clear();
}

} // namespace wge::core
