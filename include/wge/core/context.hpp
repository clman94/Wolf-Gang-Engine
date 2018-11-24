#pragma once

#include <wge/core/object_node.hpp>
#include <wge/core/layer.hpp>

#include <vector>
#include <map>

namespace wge::core
{

using instance_id = std::uint64_t;

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
		return ++mCurrent_instance_id;
	}

private:
	layer_container mLayers;
	instance_id mCurrent_instance_id{ 0 };
};

} // namespace wge::core
