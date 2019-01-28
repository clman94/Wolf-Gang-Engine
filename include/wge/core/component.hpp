#pragma once

#include <string>
#include <array>

#include <wge/math/aabb.hpp>
#include <wge/core/serialize_type.hpp>
#include <wge/core/instance_id.hpp>

#include <nlohmann/json.hpp>
#include <wge/util/json_helpers.hpp>
using json = nlohmann::json;

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
	static constexpr int COMPONENT_ID = id__; \
	static constexpr const char* COMPONENT_NAME = name__; \
	static constexpr bool COMPONENT_SINGLE_INSTANCE = false; \
	virtual int get_component_id() const override { return id__; } \
	virtual std::string get_component_name() const override { return name__; }
#define WGE_COMPONENT_SINGLE_INSTANCE(name__, id__) \
	public: \
	static constexpr int COMPONENT_ID = id__; \
	static constexpr const char* COMPONENT_NAME = name__; \
	static constexpr bool COMPONENT_SINGLE_INSTANCE = true; \
	virtual int get_component_id() const override { return id__; } \
	virtual std::string get_component_name() const override { return name__; }
#define WGE_SYSTEM_COMPONENT(name__, id__) WGE_COMPONENT_SINGLE_INSTANCE(name__, id__)

namespace wge::core
{

class context;
class game_object;
class asset_manager;

class component
{
public:
	component(component_id pId) noexcept :
		mInstance_id(pId)
	{}
	virtual ~component() {}

	// Save the current state of this component
	json serialize(serialize_type = serialize_type::all) const;
	// Load the current state of this component
	void deserialize(const game_object&, const json&);

	// Get the name of the component type
	virtual std::string get_component_name() const = 0;
	// Get the value representing the component type
	virtual int get_component_id() const = 0;

	// Unique name of component instance
	const void set_name(const std::string& pName) noexcept;
	const std::string& get_name() const noexcept;

	virtual bool has_aabb() const { return false; }
	virtual math::aabb get_screen_aabb() const { return{}; }
	virtual math::aabb get_local_aabb() const { return{}; }

	// Get the object this component is registered to
	object_id get_object_id() const noexcept;
	void set_object(const game_object& pObj) noexcept;

	component_id get_instance_id() const noexcept;

protected:
	virtual json on_serialize(serialize_type) const { return json{}; }
	virtual	void on_deserialize(const game_object& pObject, const json&) {}

private:
	std::string mName;
	object_id mObject_id;
	component_id mInstance_id;
};

} // namespace wge::core
