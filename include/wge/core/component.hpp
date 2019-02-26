#pragma once

#include <string>
#include <array>

#include <wge/math/aabb.hpp>
#include <wge/core/serialize_type.hpp>
#include <wge/core/component_type.hpp>
#include <wge/util/uuid.hpp>
#include <wge/util/json_helpers.hpp>

// Use this in your component to define the needed information the engine
// needs.
//
// class mycomponent :
//     public core::component
// {
//     WGE_COMPONENT("My Component", 32);
// public:
//     /* ... */
// };
//
#define WGE_COMPONENT(name__, id__) \
	public: \
	static constexpr wge::core::component_type COMPONENT_ID = id__; \
	static constexpr const char* COMPONENT_NAME = name__; \
	static constexpr bool COMPONENT_SINGLE_INSTANCE = false; \
	virtual const wge::core::component_type& get_component_id() const override { return COMPONENT_ID; } \
	virtual std::string get_component_name() const override { return COMPONENT_NAME; }
#define WGE_COMPONENT_SINGLE_INSTANCE(name__, id__) \
	public: \
	static constexpr wge::core::component_type COMPONENT_ID = id__; \
	static constexpr const char* COMPONENT_NAME = name__; \
	static constexpr bool COMPONENT_SINGLE_INSTANCE = true; \
	virtual const wge::core::component_type& get_component_id() const override { return COMPONENT_ID; } \
	virtual std::string get_component_name() const override { return COMPONENT_NAME; }
#define WGE_SYSTEM_COMPONENT(name__, id__) WGE_COMPONENT_SINGLE_INSTANCE(name__, id__)

namespace wge::core
{

class context;
class game_object;
class asset_manager;

class component
{
public:
	component() noexcept;
	virtual ~component() {}

	// Save the current state of this component
	json serialize(serialize_type = serialize_type::all) const;
	// Load the current state of this component.
	// Some components require extra info to completely
	// recreate its data so the game object that owns this component
	// must be provided.
	void deserialize(const game_object&, const json&);

	// Get the name of the component type
	virtual std::string get_component_name() const = 0;
	// Get the value representing the component type
	virtual const wge::core::component_type& get_component_id() const = 0;

	// Unique name of component instance
	const void set_name(const std::string& pName) noexcept;
	const std::string& get_name() const noexcept;

	// Returns true if this component can calculate an aabb
	// from its data.
	virtual bool has_aabb() const { return false; }
	// Returns the aabb in world space
	virtual math::aabb get_screen_aabb() const { return{}; }
	// Returns the aabb relative to its "center" (usually its transform).
	virtual math::aabb get_local_aabb() const { return{}; }

	// Get the object this component is registered to
	const util::uuid& get_object_id() const noexcept;
	void set_object(const game_object& pObj) noexcept;

	const util::uuid& get_instance_id() const noexcept;

	// Remove this component from its object.
	// Pointers to this component and any component of the same type are still valid until
	// the end of the frame.
	void destroy() noexcept;
	// Returns true if this component will be destroyed at the end of the frame.
	bool will_be_destroyed() const noexcept;

protected:
	virtual json on_serialize(serialize_type) const { return json{}; }
	virtual	void on_deserialize(const game_object& pObject, const json&) {}

private:
	std::string mName;
	util::uuid mObject_id;
	util::uuid mInstance_id;
	bool mWill_be_destroyed{ false };
};

} // namespace wge::core
