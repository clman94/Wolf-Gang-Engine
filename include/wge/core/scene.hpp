#pragma once

#include <wge/core/object.hpp>
#include <wge/core/layer.hpp>

#include <list>

namespace wge::core
{

class scene
{
public:
	using layers = std::list<layer>;

	// Get a layer by index
	layer* get_layer(std::size_t pIndex);

	layer& add_layer();
	layer& add_layer(const std::string& pName);

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

	void remove_object(queue_destruction_flag, const object_id& pObject_id)
	{
		if (auto obj = get_object(pObject_id))
			obj.destroy(queue_destruction);
	}

	template <typename T>
	auto get_component(object_id pObject_id, bucket pBucket = default_bucket)
	{
		if (auto obj = get_object(pObject_id))
			return obj.get_component<T>(pBucket);
		return static_cast<typename bselect_adaptor<T>::type*>(nullptr);
	}

	template <typename T>
	auto get_component(object_id pObject_id, bucket pBucket = default_bucket) const
	{
		if (auto obj = get_object(pObject_id))
			return obj.get_component<T>(pBucket);
		return static_cast<typename bselect_adaptor<T>::type const*>(nullptr);
	}

	template <typename T>
	bool remove_component(object_id pObject_id, bucket pBucket = default_bucket)
	{
		if (auto obj = get_object(pObject_id))
			return obj.remove_component<T>(pBucket);
		return false;
	}

	template <typename T>
	bool remove_component(queue_destruction_flag, object_id pObject_id)
	{
		if (auto obj = get_object(pObject_id))
			return obj.remove_component<T>(queue_destruction);
		return false;
	}

	template <typename T>
	bool remove_component(queue_destruction_flag, object_id pObject_id, bucket pBucket)
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
