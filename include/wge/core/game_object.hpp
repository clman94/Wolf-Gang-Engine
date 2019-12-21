#pragma once

#include <string>
#include <stdexcept>

#include <wge/logging/log.hpp>
#include <wge/core/serialize_type.hpp>
#include <wge/core/component_type.hpp>
#include <wge/util/json_helpers.hpp>
#include <wge/core/asset.hpp>
#include <wge/core/component.hpp>
#include <wge/core/object_id.hpp>
#include <wge/core/destruction_queue.hpp>

namespace wge::core
{

// This is used a component to hold information about objects.
// Each object is given their own.
struct object_info
{
	// An optional name for the object. Not required to be unique.
	std::string name;
	// This is the asset that was instanced by this object.
	// This will be null if this object isn't an instance of
	// an asset.
	asset::ptr source_asset;
	// For easy inspection of an object's components,
	// this list holds the types of components this object has.
	std::vector<component_type> components;
};

class layer;

// Exception thrown when a object that references no real game object
// attempts to access/modify that game object.
class invalid_game_object :
	public std::runtime_error
{
public:
	invalid_game_object() :
		std::runtime_error("Attempting to access object with invalid reference.")
	{}
};

// Game objects represent collections of components
// in a layer. This class acts as a handle
// to an object in a layer and contains only
// pointers.
class object final
{
public:
	object() noexcept;
	object(layer&, const handle<object_info>&) noexcept;

	// Check if this object has a component of a specific id
	bool has_component(const component_type& pType) const;
	// Check if this object has a component of a type
	template <class T>
	bool has_component() const;

	// Create a new component for this object
	template <typename T>
	T* add_component();
	// Get the amount of components assigned to this object.
	// Note: If a component is removed, this will not update until
	// the end of the frame.
	std::size_t get_component_count() const;
	// Get first component by type
	template <class T>
	T* get_component() const;
	// Find the first of all of these components. Returns true when all of them are found.
	template <typename Tfirst, typename...Trest>
	bool unwrap_components(Tfirst*& pFirst, Trest*& ...pRest);
	void move_component(std::size_t pFrom, std::size_t pTo);

	template <typename T>
	bool remove_component()
	{
		assert_valid_reference();
		return mLayer->remove_component<T>(mInfo.get_object_id());
	}

	template <typename T>
	bool remove_component(queue_destruction_flag)
	{
		assert_valid_reference();
		return mLayer->remove_component<T>(mInfo.get_object_id(), queue_destruction);
	}

	// Get the name of this object.
	const std::string& get_name() const;
	// Set the name of this object. Use this to give meaningful identifiers
	// to objects.
	void set_name(const std::string& pName);

	// Remove this object from the layer.
	// It is recommended that you discard any object objects because this
	// function will remove the data they are pointing to, thus leaving
	// them in an invalid state.
	void destroy();
	void destroy(queue_destruction_flag);

	// Get a reference to the layer this object belongs to.
	// To actually move an object to a new layer, you will need to
	// reconstruct this object in the destination layer and remove it in
	// the source layer.
	layer& get_layer() const;

	bool is_instanced() const noexcept;
	void set_asset(const core::asset::ptr&) noexcept;
	asset::ptr get_asset() const;

	// Get the id that uniquely identifies this object.
	object_id get_id() const;

	operator bool() const noexcept
	{
		return is_valid();
	}

	// Returns true if this is a valid reference to an object.
	bool is_valid() const noexcept
	{
		return mInfo.is_valid() && mLayer != nullptr;
	}

	void reset() noexcept
	{
		mLayer = nullptr;
		mInfo.reset();
	}

	bool operator==(const object& pObj) const noexcept;

private:
	void assert_valid_reference() const;

private:
	layer* mLayer;
	handle<object_info> mInfo;
};

template<class T>
inline bool object::has_component() const
{
	return has_component(component_type::from<T>());
}

template<typename T>
inline T* object::add_component()
{
	assert_valid_reference();
	return get_layer().add_component<T>(get_id());
}

template<class T>
inline T* object::get_component() const
{
	return mLayer->get_storage<T>().get(get_id());
}

template<typename Tfirst, typename ...Trest>
inline bool object::unwrap_components(Tfirst*& pFirst, Trest*& ...pRest)
{
	assert_valid_reference();
	auto comp = get_component<Tfirst>();
	if (!comp)
		return false;
	pFirst = comp;
	if constexpr (sizeof...(pRest) == 0)
		return true;
	else
		return unwrap_components(pRest...);
}

} // namespace wge::core
