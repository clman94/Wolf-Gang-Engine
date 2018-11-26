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
class game_object;

// Game objects represent collections of components
// in a layer. This class mainly acts like a handle
// to an object in a layer and generally contains only
// pointers.
class game_object
{
public:
	game_object(layer&);
	game_object(layer&, instance_id);

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
		WGE_ASSERT(mInstance_id);
		return get_layer().add_component<T>(*this);
	}

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

	// Remove this object from the layer.
	// It is recommended that you discard this object because this
	// function will leave it in an invalid state.
	void destroy();

	layer& get_layer() const;

	instance_id get_instance_id() const
	{
		return mInstance_id;
	}

	void set_instance_id(instance_id pId)
	{
		mInstance_id = pId;
	}

	operator bool() const
	{
		return mInstance_id.is_valid();
	}

	void reset()
	{
		mInstance_id = 0;
	}

private:
	std::reference_wrapper<layer> mLayer;
	instance_id mInstance_id;
};

} // namespace wge::core
