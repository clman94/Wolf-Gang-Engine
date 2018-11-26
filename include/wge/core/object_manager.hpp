#pragma once

#include <wge/core/game_object.hpp>
#include <wge/core/component.hpp>
#include <vector>
#include <utility>
#include <optional>

namespace wge::core
{

struct object_data
{
	object_data(object_id pId);
	std::string name;
	object_id id;
	std::vector<component*> components;
};

class object_manager
{
public:
	object_data& add_object(object_id pId);
	void remove_object(object_id pId);

	std::size_t get_object_count() const;

	// Registers a component
	void register_component(component* pComponent);
	// Unregister a component
	void unregister_component(component* pComponent);

	// Get object data by object id
	object_data* get_object_data(object_id pId);
	// Get object data by index
	object_data* get_object_data(std::size_t pIndex);

private:
	std::vector<object_data> mObjects;
};

} // namespace wge::core
