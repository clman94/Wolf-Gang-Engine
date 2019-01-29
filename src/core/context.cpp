#include <wge/core/context.hpp>

namespace wge::core
{

layer::ptr context::get_layer(std::size_t pIndex) const
{
	if (pIndex >= mLayers.size())
		return{};
	return mLayers[pIndex];
}

layer::ptr context::create_layer()
{
	return mLayers.emplace_back(layer::create(*this));
}

layer::ptr context::create_layer(const std::string& pName)
{
	auto l = create_layer();
	l->set_name(pName);
	return l;
}

layer::ptr context::create_layer(const std::string& pName, std::size_t pInsert)
{
	mLayers.insert(mLayers.begin() + pInsert, layer::create(*this));
	return mLayers[pInsert];
}

const context::layer_container& context::get_layer_container() const noexcept
{
	return mLayers;
}

instance_id_t context::get_unique_instance_id() noexcept
{
	return ++mCurrent_instance_id;
}

void context::set_asset_manager(asset_manager * pAsset_manager) noexcept
{
	mAsset_manager = pAsset_manager;
}

asset_manager* context::get_asset_manager() const noexcept
{
	return mAsset_manager;
}

void context::set_factory(factory* pFactory) noexcept
{
	mFactory = pFactory;
}

factory* context::get_factory() const noexcept
{
	return mFactory;
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

} // namespace wge::core