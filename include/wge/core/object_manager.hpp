#pragma once

#include <wge/core/game_object.hpp>
#include <wge/core/component.hpp>
#include <list>
#include <utility>
#include <optional>

namespace wge::core
{

struct object_data
{
	object_data(object_id pId);
	std::string name;
	object_id id;
	struct component_entry
	{
		int type;
		component_id id;
	};
	std::vector<component_entry> components;
};

class object_manager
{
public:
	object_data& add_object(object_id pId);
	void remove_object(object_id pId);

	std::size_t get_object_count() const noexcept;

	// Registers a component
	void register_component(component* pComponent);
	// Unregister a component
	void unregister_component(component* pComponent);
	void unregister_component(object_id pObj_id, component_id pComp_id);

	// Get object data by object id
	object_data* get_object_data(object_id pId);
	// Get object data by index
	object_data* get_object_data(std::size_t pIndex);

private:
	std::list<object_data> mObjects;
};

} // namespace wge::core
