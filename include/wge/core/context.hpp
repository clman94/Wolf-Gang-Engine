#pragma once

#include <wge/core/instance_id.hpp>
#include <wge/core/layer.hpp>

#include <vector>
#include <map>

namespace wge::core
{

class asset_manager;

class context
{
public:
	using layer_container = std::vector<layer::ptr>;

	// Get a layer to a specific index
	layer::ptr get_layer(std::size_t pIndex) const
	{
		if (pIndex >= mLayers.size())
			return{};
		return mLayers[pIndex];
	}

	layer::ptr create_layer()
	{
		return mLayers.emplace_back(layer::create(*this));
	}

	layer::ptr create_layer(const std::string& pName)
	{
		auto l = create_layer();
		l->set_name(pName);
		return l;
	}

	layer::ptr create_layer(const std::string& pName, std::size_t pInsert)
	{
		mLayers.insert(mLayers.begin() + pInsert, layer::create(*this));
		return mLayers[pInsert];
	}

	const layer_container& get_layer_container() const noexcept
	{
		return mLayers;
	}

	instance_id_t get_unique_instance_id() noexcept
	{
		return ++mCurrent_instance_id;
	}

	void set_asset_manager(asset_manager* pAsset_manager) noexcept
	{
		mAsset_manager = pAsset_manager;
	}

	asset_manager* get_asset_manager() const noexcept
	{
		return mAsset_manager;
	}

	void preupdate(float pDelta)
	{
		for (auto& i : mLayers)
			i->preupdate(pDelta);
	}

	void update(float pDelta)
	{
		for (auto& i : mLayers)
			i->update(pDelta);
	}

	void postupdate(float pDelta)
	{
		for (auto& i : mLayers)
			i->postupdate(pDelta);
	}

private:
	layer_container mLayers;
	instance_id_t mCurrent_instance_id{ 0 };
	asset_manager* mAsset_manager{ nullptr };
};

} // namespace wge::core
