#pragma once

#include <wge/core/object.hpp>
#include <wge/core/layer.hpp>

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

	object get_object(const object_id& pObject_id)
	{
		for (auto& i : mLayers)
			if (auto obj = i.get_object(pObject_id))
				return obj;
		return invalid_object;
	}

	void remove_object(const object_id& pObject_id)
	{
		if (auto obj = get_object(pObject_id))
			obj.destroy();
	}

	void remove_object(const object_id& pObject_id, queue_destruction_flag)
	{
		if (auto obj = get_object(pObject_id))
			obj.destroy(queue_destruction);
	}

	template <typename T>
	T* get_component(object_id pObject_id, bucket pBucket = default_bucket)
	{
		if (auto obj = get_object(pObject_id))
			return obj.get_component<T>(pBucket);
		return nullptr;
	}

	template <typename T>
	const T* get_component(object_id pObject_id, bucket pBucket = default_bucket) const
	{
		if (auto obj = get_object(pObject_id))
			return obj.get_component<T>(pBucket);
		return nullptr;
	}

	template <typename T>
	bool remove_component(object_id pObject_id, bucket pBucket = default_bucket)
	{
		if (auto obj = get_object(pObject_id))
			return obj.remove_component<T>(pBucket);
		return false;
	}

	template <typename T>
	bool remove_component(object_id pObject_id, queue_destruction_flag)
	{
		if (auto obj = get_object(pObject_id))
			return obj.remove_component<T>(queue_destruction);
		return false;
	}

	template <typename T>
	bool remove_component(object_id pObject_id, bucket pBucket, queue_destruction_flag)
	{
		if (auto obj = get_object(pObject_id))
			return obj.remove_component<T>(pBucket, queue_destruction);
		return false;
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
