#pragma once

#include <string>
#include <vector>
#include <memory>

#include <wge/core/asset_config.hpp>

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
class object_node;

// Object nodes are objects that relay
// messages between components and to other 
// game objects.
class object_node
{
public:
	object_node(layer&);
	~object_node();

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

	// Get component by name
	component* get_component(const std::string& pName);
	// Get first component by id
	component* get_component(int pId) const;
	// Get component by index
	component* get_component_index(std::size_t pIndex) const;
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

	layer& get_layer() const;

private:
	std::reference_wrapper<layer> mLayer;
	instance_id mInstance_id;
};

} // namespace wge::core
