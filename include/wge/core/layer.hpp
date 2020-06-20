#pragma once

#include <wge/core/game_object.hpp>
#include <wge/core/component_set.hpp>
#include <wge/core/component_manager.hpp>
#include <wge/core/component_type.hpp>
#include <wge/core/destruction_queue.hpp>
#include <wge/util/ptr.hpp>

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>

namespace wge::core
{

// A layer is a container of objects and their components.
//
class layer final
{
public:
	class iterator
	{
		friend class layer;
		using storage_iterator = component_storage<object_info>::iterator;

		iterator(layer& pLayer, storage_iterator& pIter) :
			mLayer(&pLayer),
			mIter(pIter)
		{}

	public:
		iterator() = default;

		object get() const
		{
			return mLayer->get_object(mIter->first);
		}

		void next()
		{
			++mIter;
		}

		iterator& operator++()
		{
			next();
			return *this;
		}

		iterator operator++(int)
		{
			iterator temp = *this;
			next();
			return temp;
		}

		bool operator==(const iterator& pR) const
		{
			return mIter == pR.mIter;
		}

		bool operator!=(const iterator& pR) const
		{
			return mIter != pR.mIter;
		}

		auto operator->() const
		{
			return util::ptr_adaptor{ get() };
		}

		object operator*() const
		{
			return get();
		}

	private:
		layer* mLayer = nullptr;
		storage_iterator mIter;
	};

	// Set the name of this layer
	void set_name(const std::string& pName) noexcept;
	// Get the name of this layer
	const std::string& get_name() const noexcept;

	// Create a new object in this layer.
	[[nodiscard]] object add_object();
	object add_object(const std::string& pName);
	object get_object(object_id pId)
	{
		return object{ *this, make_handle<object_info>(pId) };
	}
	object get_object_at(std::size_t pIndex)
	{
		return get_object(mComponent_manager.get_storage<object_info>().at(pIndex).first);
	}
	// Remove a game object
	void remove_object(const object_id& pObject_id);
	void remove_object(const object_id& pObject_id, queue_destruction_flag);
	void remove_all_objects();
	
	// Pointers to components can expire. If a reference to
	// a component is required, use this function to create a
	// handle that won't expire until the component it points to
	// is destroyed.
	template <typename T>
	auto make_handle(object_id pId, bucket pBucket = default_bucket)
	{
		return handle<T>{ pId, mComponent_manager.get_storage<T>(pBucket) };
	}

	// Get the amount of game objects in this layer.
	std::size_t get_object_count() const noexcept;

	// Add a new component to an object.
	template <typename T>
	auto* add_component(const object_id& pObject_id, bucket pBucket = default_bucket);

	// Add a new component to an object.
	template <typename T, typename U = std::decay_t<T>,
		// Buckets can't be used as components.
		typename = std::enable_if_t<!std::is_same_v<U, bucket>>>
	auto* add_component(const object_id& pObject_id, T&& pComponent, bucket pBucket = default_bucket)
	{
		return &mComponent_manager.add_component(pObject_id, std::forward<T>(pComponent), pBucket);
	}

	template <typename T>
	auto* get_component(object_id pObject_id, bucket pBucket = default_bucket)
	{
		return mComponent_manager.get_component<T>(pObject_id, pBucket);
	}

	template <typename T>
	const auto* get_component(object_id pObject_id, bucket pBucket = default_bucket) const
	{
		return mComponent_manager.get_component<T>(pObject_id, pBucket);
	}

	template <typename T>
	bool remove_component(object_id pObject_id, bucket pBucket = default_bucket)
	{
		mComponent_manager.remove_component<T>(pObject_id, pBucket);
	}

	template <typename T>
	bool remove_component(object_id pObject_id, queue_destruction_flag)
	{
		remove_component<T>(pObject_id, default_bucket, queue_destruction);
	}

	template <typename T>
	bool remove_component(object_id pObject_id, bucket pBucket, queue_destruction_flag)
	{
		mDestruction_queue.push_component(pObject_id, component_type::from<T>(pBucket));
	}

	// Get the container associated to a specific type of component.
	template <typename T>
	auto& get_storage(bucket pBucket = default_bucket)
	{
		return mComponent_manager.get_storage<T>(pBucket);
	}

	// Get the container associated to a specific type of component.
	template <typename T>
	const auto& get_storage(bucket pBucket = default_bucket) const
	{
		return mComponent_manager.get_storage<T>(pBucket);
	}

	template <typename T, typename...Tdeps>
	auto each();

	auto begin()
	{
		return iterator{ *this, get_storage<object_info>().begin() };
	}

	auto end()
	{
		return iterator{ *this, get_storage<object_info>().end() };
	}

	// Calls pCallable for each component specified by the first parameter.
	// Each component parameter should be a non-const reference and derive from
	// the component class.
	// Example:
	//    // This will iterate over each sprite component in the layer.
	//    layer.for_each([&](sprite_component& pTarget) {});
	//    
	//    // This will iterate over each sprite component and will collect
	//    // the first transform component of the owning object.
	//    layer.for_each([&](sprite_component& pTarget, transform_component& pTransform) {});
	//    
	//    // You can also get the current game object by adding a `game_object` parameter at the start.
	//    layer.for_each([&](object pObject, sprite_component& pTarget) {});
	//
	template <typename T>
	void for_each(T&& pCallable);

	// Set whether or not this layer will recieve updates.
	// Note: This does not affect the update methods in this class
	//   so they are still callable.
	void set_enabled(bool pEnabled) noexcept;
	// Check if this layer is eligible to recieve updates.
	bool is_enabled() const noexcept;

	// Set the scale for the delta parameters in the update methods.
	void set_time_scale(float pScale) noexcept;
	// Get the scale for the delta parameters in the update methods.
	float get_time_scale() const noexcept;

	void clear();

	void destroy_queued_components()
	{
		mDestruction_queue.apply(mComponent_manager);
	}

	// These components are for the layer.
	// With this, the layer is pretty much an object itself.
	// Made public for convenience.
	component_set layer_components;

private:
	template <typename T, typename...Tdeps>
	void for_each_impl(const std::function<void(object_id, T, Tdeps...)>& pCallable);

	float mTime_scale{ 1 };
	bool mRecieve_update{ true };
	std::string mName;
	// Because we can't remove components while iterating them
	// we can queue those components to be removed after the iteration
	// is complete.
	destruction_queue mDestruction_queue;
	// This manages the components for the objects.
	component_manager mComponent_manager;
};

// A range object that filters out all objects that
// don't have the following components.
//
// This runs in O(n) where n is the amount of
// elements in the provided component_storage<T>.
// The amount of Tdeps does not affect this.
template <typename T, typename...Tdeps>
class filter
{
public:
	class iterator
	{
	public:
		using storage_iterator = typename component_storage<T>::iterator;
		iterator() = default;
		explicit iterator(
			storage_iterator pIter,
			storage_iterator pEnd,
			component_storage<Tdeps>&...pDeps_storage) :
			mIter(pIter),
			mEnd(pEnd),
			mDeps_storage(&pDeps_storage...)
		{
			// "Prime" the iterator by finding the first
			// element with the matching components.
			while (mIter != mEnd && !find_dependencies())
				++mIter;
		}

		using value_type = std::tuple<object_id, T&, Tdeps&...>;

		auto get() const noexcept
		{
			return value_type{ mIter->first, mIter->second, *std::get<Tdeps*>(mDeps)... };
		}

		auto operator->() const noexcept
		{
			return util::ptr_adaptor{ get() };
		}

		auto operator*() const noexcept
		{
			return get();
		}

		void next()
		{
			do {
				++mIter;
			} while (mIter != mEnd && !find_dependencies());
		}

		bool operator==(const iterator& pR) const noexcept
		{
			return mIter == pR.mIter;
		}

		bool operator!=(const iterator& pR) const noexcept
		{
			return mIter != pR.mIter;
		}

		iterator& operator++() noexcept
		{
			next();
			return *this;
		}

		iterator operator++(int) noexcept
		{
			iterator temp = *this;
			next();
			return temp;
		}

	private:
		bool find_dependencies() noexcept
		{
			if constexpr (sizeof...(Tdeps) != 0)
			{
				const object_id& id = mIter->first;
				mDeps = { std::get<component_storage<Tdeps>*>(mDeps_storage)->get(id)... };
				if (!(std::get<Tdeps*>(mDeps) && ...))
					return false;
			}
			return true;
		}

		std::tuple<Tdeps*...> mDeps;
		storage_iterator mIter;
		storage_iterator mEnd;
		std::tuple<component_storage<Tdeps>*...> mDeps_storage;
	};

	template <typename T, typename...Tdeps> // hmmm
	explicit filter(component_storage<T>& pContainer,
		component_storage<Tdeps>&...pDeps) :
		mBegin(pContainer.begin(), pContainer.end(), pDeps...),
		mEnd(pContainer.end(), pContainer.end(), pDeps...)
	{}

	auto begin() const noexcept
	{
		return mBegin;
	}

	auto end() const noexcept
	{
		return mEnd;
	}

private:
	iterator mBegin, mEnd;
};

template <typename T, typename...Tdeps>
filter(component_storage<T>&, component_storage<Tdeps>&...) -> filter<T, Tdeps...>;

template<typename T>
inline auto* layer::add_component(const object_id& pObject_id, bucket pBucket)
{
	return &mComponent_manager.add_component<T>(pObject_id, pBucket);
}

template<typename T, typename...Tdeps>
inline auto layer::each()
{
	return filter{ get_storage<T>(), get_storage<Tdeps>()... };
}

template <typename T, typename...Tdeps>
inline void layer::for_each_impl(const std::function<void(object_id, T, Tdeps...)>& pCallable)
{
	for (auto i : each<T, Tdeps...>())
		std::apply(pCallable, i);
}

template<typename T>
inline void layer::for_each(T&& pCallable)
{
	for_each(std::function{ pCallable });
}

} // namespace wge::core
