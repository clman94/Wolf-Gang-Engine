#pragma once

#include <wge/core/game_object.hpp>
#include <wge/core/component.hpp>
#include <wge/util/uuid.hpp>
#include <list>
#include <utility>
#include <optional>

namespace wge::core
{

using object_tracker = std::shared_ptr<bool>;

class object_data
{
public:
	object_data();
	~object_data();

	std::string name;
	util::uuid id;

	struct component_entry
	{
		component_type type;
		util::uuid id;
	};
	std::vector<component_entry> components;
	
	const object_tracker& get_tracker() const noexcept
	{
		return mTracker;
	}

private:
	object_tracker mTracker;
};

// Organizes object data
class object_manager
{
public:
	object_data& add_object();
	void remove_object(const util::uuid& pId);

	std::size_t get_object_count() const noexcept;

	bool has_object(const util::uuid& pId) const noexcept;

	// Registers a component
	void register_component(component* pComponent);
	// Unregister a component
	void unregister_component(component* pComponent);
	void unregister_component(const util::uuid& pObj_id, const util::uuid& pComp_id);

	// Get object data by object id
	object_data* get_object_data(const util::uuid& pId);
	// Get object data by index
	object_data* get_object_data(std::size_t pIndex);

private:
	std::list<object_data> mObjects;
};

} // namespace wge::core
