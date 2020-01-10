#pragma once

#include <wge/core/layer.hpp>
#include <wge/core/factory.hpp>

#include <list>

namespace wge::core
{

class scene
{
public:
	using layers = std::list<layer>;

	// Get a layer by index
	layer* get_layer(std::size_t pIndex);

	layer* add_layer();
	layer* add_layer(const layer& pLayer)
	{
		mLayers.push_back(pLayer);
		return &mLayers.back();
	}
	layer* add_layer(layer&& pLayer)
	{
		mLayers.push_back(std::move(pLayer));
		return &mLayers.back();
	}
	layer* add_layer(const std::string& pName);

	// Returns true if the layer was successfully removed.
	bool remove_layer(const layer& pPtr);

	layers& get_layer_container() noexcept;

	void clear();

	auto begin() { return mLayers.begin(); }
	auto end() { return mLayers.end(); }
	auto begin() const { return mLayers.begin(); }
	auto end() const { return mLayers.end(); }

private:
	layers mLayers;
};

} // namespace wge::core
