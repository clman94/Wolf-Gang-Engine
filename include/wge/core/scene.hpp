#pragma once

#include <wge/core/game_object.hpp>
#include <wge/core/layer.hpp>
#include <wge/core/factory.hpp>

#include <list>

namespace wge::core
{

class scene
{
public:
	// You may be thinking "wouldn't vector be fine here?"
	// Unfortunately, that is not "always" the case.
	// Layer is a pretty heavy object to move
	// around and combine that with the fact that vector
	// doesn't always move on some platforms and compilers.
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

	object get_object(const object_id& pId)
	{
		for (auto& i : mLayers)
			if (auto obj = i.get_object(pId))
				return obj;
	}

	void remove_object(const object_id& pId)
	{
		if (auto obj = get_object(pId))
			obj.destroy();
	}

	void remove_object(const object_id& pId, queue_destruction_flag)
	{
		if (auto obj = get_object(pId))
			obj.destroy(queue_destruction);
	}

	void clear();

	auto begin() { return mLayers.begin(); }
	auto end() { return mLayers.end(); }
	auto begin() const { return mLayers.begin(); }
	auto end() const { return mLayers.end(); }

private:
	layers mLayers;
};

} // namespace wge::core
