#pragma once

#include <string>
#include <vector>
#include <memory>

#include <wge/logging/log.hpp>
#include <wge/core/asset_config.hpp>
#include <wge/core/instance_id.hpp>
#include <wge/util/json_helpers.hpp>

namespace wge::core
{

// This template is used to check if a type has the "COMPONENT_ID" member.
template <typename T, typename = int>
struct has_component_id_member : std::false_type {};
template <typename T>
struct has_component_id_member <T, decltype((void)T::COMPONENT_ID, 0)> : std::true_type {};

class layer;
class component;
struct object_data;

// Game objects represent collections of components
// in a layer. This class mainly acts as a handle
// to an object in a layer and generally contains only
// pointers.
class game_object
{
public:
	game_object(layer&);
	game_object(layer&, object_data&);

	// Check if this object has a component of a specific id
	bool has_component(int pId) const;
	// Check if this object has a component of a type
	template<class T,
		// Requires the "int COMPONENT_ID" member
		typename = std::enable_if<has_component_id_member<T>::value>::type>
	bool has_component() const
	{
		return has_component(T::COMPONENT_ID);
	}

	template <typename T>
	auto add_component()
	{
		WGE_ASSERT(mData);
		return get_layer().add_component<T>(*this);
	}

	std::size_t get_component_count() const;
	component* get_component_index(std::size_t pIndex);
	// Get component by name
	component* get_component(const std::string& pName);
	// Get first component by id
	component* get_component(int pId) const;
	// Get first component by type
	template<class T,
		// Requires the "int COMPONENT_ID" COMPONENT_ID member
		typename = std::enable_if<has_component_id_member<T>::value>::type>
	T* get_component() const
	{
		return static_cast<T*>(get_component(T::COMPONENT_ID));
	}
	// Remove component at index
	void remove_component(std::size_t pIndex);
	// Remove all components
	void remove_components();

	const std::string& get_name() const;
	void set_name(const std::string& pName);

	// Remove this object from the layer.
	// It is recommended that you discard this object because this
	// function will leave it in an invalid state.
	void destroy();

	layer& get_layer() const;

	object_id get_instance_id() const;

	void set_instance_id(object_id pId);

	operator bool() const
	{
		return mData != nullptr;
	}

	void reset()
	{
		mData = nullptr;
	}

private:
	std::reference_wrapper<layer> mLayer;
	object_data* mData;
};

} // namespace wge::core
