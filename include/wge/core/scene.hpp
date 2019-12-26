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
	using uptr = std::unique_ptr<scene>;
	using layers = std::deque<layer>;

	// Get a layer by index
	[[nodiscard]] layer* get_layer(std::size_t pIndex);

	layer* add_layer();
	layer* add_layer(const layer& pLayer)
	{
		mLayers.push_back(pLayer);
		return &mLayers.back();
	}
	layer* add_layer(layer&& pLayer)
	{
		return &mLayers.emplace_back(std::move(pLayer));
	}
	layer* add_layer(const std::string& pName);
	layer* add_layer(const std::string& pName, std::size_t pInsert);

	// Returns true if the layer was successfully removed.
	bool remove_layer(const layer& pPtr);

	layers& get_layer_container() noexcept;

	void clear();

private:
	layers mLayers;
};

} // namespace wge::core
