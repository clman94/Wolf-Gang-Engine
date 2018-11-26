#pragma once

#include <wge/core/object_node.hpp>
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

	const layer_container& get_layer_container() const
	{
		return mLayers;
	}

	instance_id get_unique_instance_id()
	{
		mCurrent_instance_id.set_value(mCurrent_instance_id.get_value() + 1);
		return mCurrent_instance_id;
	}

	void set_asset_manager(asset_manager* pAsset_manager)
	{
		mAsset_manager = pAsset_manager;
	}

	asset_manager* get_asset_manager() const
	{
		return mAsset_manager;
	}

private:
	layer_container mLayers;
	instance_id mCurrent_instance_id{ 0 };
	asset_manager* mAsset_manager;
};

} // namespace wge::core
