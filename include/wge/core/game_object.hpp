#pragma once

#include <string>
#include <stdexcept>

#include <wge/logging/log.hpp>
#include <wge/core/serialize_type.hpp>
#include <wge/core/component_type.hpp>
#include <wge/util/json_helpers.hpp>
#include <wge/core/asset.hpp>

namespace wge::core
{

// This template is used to check if a type has the "COMPONENT_ID" member.
template <typename T, typename = int>
struct has_component_id_member : std::false_type {};
template <typename T>
struct has_component_id_member <T, decltype((void)T::COMPONENT_ID, 0)> : std::true_type {};

class scene;
class layer;
class component;
class object_data;

// Exception thrown when a game_object that references no real game object
// attempts to access/modify that game object.
class invalid_game_object :
	public std::runtime_error
{
public:
	invalid_game_object() :
		std::runtime_error("Attempting to access game object with invalid reference.")
	{}
};

// Game objects represent collections of components
// in a layer. This class mainly acts as a handle
// to an object in a layer and generally contains only
// pointers.
class game_object
{
public:
	game_object() noexcept;
	game_object(layer&, object_data&) noexcept;

	// Check if this object has a component of a specific id
	bool has_component(const component_type& pType) const;
	// Check if this object has a component of a type
	template <class T,
		// Requires the "int COMPONENT_ID" member
		typename = std::enable_if<has_component_id_member<T>::value>::type>
	bool has_component() const;

	json serialize(serialize_type pType = serialize_type::all) const;
	void deserialize(const json& pJson);
	void deserialize(const asset::ptr& pAsset);

	// Create a new component for this object
	template <typename T>
	T* add_component();
	component* add_component(const component_type& pType);
	// Get the amount of components assigned to this object.
	// Note: If a component is removed, this will not update until
	// the end of the frame.
	std::size_t get_component_count() const;
	// Get a component by its index
	component* get_component_at(std::size_t pIndex);
	// Get component by name
	component* get_component(const std::string& pName);
	component* get_component_by_type(const std::string& pName);
	// Get first component by id
	component* get_component(const component_type& pType) const;
	// Get first component by type
	template <class T,
		// Requires the "int COMPONENT_ID" COMPONENT_ID member
		typename = std::enable_if<has_component_id_member<T>::value>::type>
	T* get_component() const;
	// Find the first of all of these components. Returns true when all of them are found.
	template <typename Tfirst, typename...Trest>
	bool unwrap_components(Tfirst*& pFirst, Trest*& ...pRest);
	void move_component(std::size_t pFrom, std::size_t pTo);
	// Remove component at index
	void remove_component(std::size_t pIndex);
	// Remove all components
	void remove_all_components();

	// Get the name of this object.
	const std::string& get_name() const;
	// Set the name of this object. Use this to give meaningful identifiers
	// to objects.
	void set_name(const std::string& pName);

	// Remove this object from the layer.
	// It is recommended that you discard any game_object objects because this
	// function will remove the data they are pointing to, thus leaving
	// them in an invalid state. TODO: Add some form of tracking.
	void destroy();

	// Get a reference to the layer this object belongs to.
	// To actually move an object to a new layer, you will need to
	// reconstruct this object in the destination layer and remove it in
	// the source layer.
	layer& get_layer() const;
	scene& get_scene() const;

	bool is_instanced() const noexcept;
	asset::ptr get_asset() const;

	// Get the id that uniquely identifies this object
	const util::uuid& get_instance_id() const;

	operator bool() const noexcept
	{
		return is_valid();
	}

	// Returns true if this is a valid reference to an object.
	bool is_valid() const noexcept
	{
		return mData != nullptr;
	}

	void reset() noexcept
	{
		mLayer = nullptr;
		mData = nullptr;
	}

	bool operator==(const game_object& pObj) const noexcept;

private:
	void assert_valid_reference() const;

private:
	layer* mLayer;
	object_data* mData;
};

template<class T, typename>
inline bool game_object::has_component() const
{
	return has_component(T::COMPONENT_ID);
}

template<typename T>
inline T* game_object::add_component()
{
	assert_valid_reference();
	return get_layer().add_component<T>(*this);
}

template<class T, typename>
inline T* game_object::get_component() const
{
	return static_cast<T*>(get_component(T::COMPONENT_ID));
}

template<typename Tfirst, typename ...Trest>
inline bool game_object::unwrap_components(Tfirst*& pFirst, Trest*& ...pRest)
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
