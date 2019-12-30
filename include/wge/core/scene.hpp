#pragma once

#include <wge/core/layer.hpp>
#include <wge/core/factory.hpp>

#include <memory>
#include <vector>
#include <map>
#include <utility>
#include <tuple>

namespace wge::core
{

class scene
{
public:
	using layers = std::vector<util::copyable_ptr<layer>>;

	// Get a layer by index
	layer* get_layer(std::size_t pIndex);

	layer* add_layer();
	layer* add_layer(const layer& pLayer)
	{
		mLayers.push_back(util::make_copyable_ptr<layer>(pLayer));
		return mLayers.back().get();
	}
	layer* add_layer(layer&& pLayer)
	{
		return mLayers.emplace_back(util::make_copyable_ptr<layer>(std::move(pLayer))).get();
	}
	layer* add_layer(const std::string& pName);

	// Returns true if the layer was successfully removed.
	bool remove_layer(const layer& pPtr);

	layers& get_layer_container() noexcept;

	void clear();

private:
	layers mLayers;
};

} // namespace wge::core
