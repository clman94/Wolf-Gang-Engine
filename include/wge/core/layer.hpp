#pragma once

#include <wge/core/game_object.hpp>
#include <wge/core/system.hpp>
#include <wge/core/component_manager.hpp>
#include <wge/core/component_type.hpp>
#include <wge/core/factory.hpp>
#include <wge/core/destruction_queue.hpp>
#include <wge/util/ptr.hpp>

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>

namespace wge::core
{

// A layer is a self-contained collection of objects
// with its own set of systems acting upon the components
// those objects.
// In short, this is where all the ECS goodies happen.
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

	using uptr = std::unique_ptr<layer>;

	// Create a new layer object
	[[nodiscard]] static uptr create(const factory& pFactory)
	{
		return std::make_unique<layer>(pFactory);
	}
	layer() = default;
	layer(const factory&) noexcept;

	// Get a system by its type
	template <typename T>
	T* get_system() const;
	// Get a system by its type
	system* get_system(int pID) const;
	// Get a system by name
	system* get_system(const std::string& pName) const;

	// Create a system that was not registered in the factory.
	template <typename T, typename...Targs>
	T* add_unregistered_system(Targs&&...);
	// Uses the current factory to construct the system.
	template <typename T>
	T* add_system();
	// Create a new system that was registered in the factory.
	system* add_system(int pType);

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
	auto make_handle(object_id pId)
	{
		return handle{ pId, mComponent_manager.get_storage<T>() };
	}

	// Get the amount of game objects in this layer
	std::size_t get_object_count() const noexcept;

	// Add a new component to an object
	template <typename T>
	T* add_component(const object_id& pObject_id);

	template <typename T>
	T* get_component(object_id pObject_id)
	{
		return mComponent_manager.get_component<T>(pObject_id);
	}

	template <typename T>
	bool remove_component(object_id pObject_id)
	{
		mComponent_manager.remove_component<T>(pObject_id);
	}

	template <typename T>
	bool remove_component(object_id pObject_id, queue_destruction_flag)
	{
		mDestruction_queue.push_component(pObject_id, component_type::from<T>());
	}

	// Get the container associated to a specific type of component.
	template <typename T>
	component_storage<T>& get_storage();

	// Get the container associated to a specific type of component.
	template <typename T>
	const component_storage<T>& get_storage() const;

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

private:
	template <typename T, typename...Tdeps>
	void for_each_impl(const std::function<void(object_id, T, Tdeps...)>& pCallable);

	const factory* mFactory = nullptr;
	float mTime_scale{ 1 };
	bool mRecieve_update{ true };
	std::string mName;
	std::vector<util::copyable_ptr<system>> mSystems;
	destruction_queue mDestruction_queue;
	component_manager mComponent_manager;
};

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
		bool find_dependencies() const noexcept
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

		mutable std::tuple<Tdeps*...> mDeps;
		storage_iterator mIter;
		storage_iterator mEnd;
		std::tuple<component_storage<Tdeps>*...> mDeps_storage;
	};

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

template<typename T>
inline T* layer::get_system() const
{
	return dynamic_cast<T*>(get_system(T::SYSTEM_ID));
}

template<typename T, typename ...Targs>
inline T* layer::add_unregistered_system(Targs&&...pArgs)
{
	return static_cast<T*>(mSystems.emplace_back(util::make_copyable_ptr<T, system>(std::forward<Targs>(pArgs)...)).get());
}

template<typename T>
inline T* layer::add_system()
{
	assert(mFactory);
	return static_cast<T*>(add_system(T::SYSTEM_ID));
}

template<typename T>
inline T* layer::add_component(const object_id& pObject_id)
{
	return &mComponent_manager.add_component<T>(pObject_id);
}

template<typename T>
inline component_storage<T>& layer::get_storage()
{
	return mComponent_manager.get_storage<T>();
}
template<typename T>
inline const component_storage<T>& layer::get_storage() const
{
	return mComponent_manager.get_storage<T>();
}

template<typename T, typename...Tdeps>
inline auto layer::each()
{
	return filter<T, Tdeps...>{ get_storage<T>(), get_storage<Tdeps>()... };
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
