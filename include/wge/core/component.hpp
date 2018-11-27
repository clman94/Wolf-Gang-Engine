#pragma once

#include <string>
#include <array>

#include <wge/math/aabb.hpp>
#include <wge/core/messaging.hpp>
#include <wge/core/serializable.hpp>
#include <wge/core/instance_id.hpp>

#include <nlohmann/json.hpp>
#include <wge/util/json_helpers.hpp>
using json = nlohmann::json;

// Use this in your class to define the needed information the engine
// needs about your component.
//
// class mycomponent :
//     public core::component
// {
//     WGE_COMPONENT("My Component", 32);
// public:
//     ...
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
	component(component_id pId):
		mInstance_id(pId)
	{}
	virtual ~component() {}

	// Save the configuration of this component to json
	virtual json save() { return {}; }
	// Load the configuration of this conponent from json
	virtual void load(const json&) {}

	// Save the current state of this component
	virtual json serialize() const { return {}; }
	// Load the current state of this component
	virtual void deserialize(const json&) {}

	// Name of the component type
	virtual std::string get_component_name() const = 0;
	// Id of the component type for serialization
	virtual int get_component_id() const = 0;

	// Unique name of component instance
	const void set_name(const std::string& pName);
	const std::string& get_name() const;

	virtual bool has_aabb() const { return false; }
	virtual math::aabb get_screen_aabb() const { return{}; };

	object_id get_object_id() const
	{
		return mObject_id;
	}

	void set_object(const game_object& pObj);

	component_id get_instance_id() const
	{
		return mInstance_id;
	}

private:
	std::string mName;
	object_id mObject_id;
	component_id mInstance_id;
};

} // namespace wge::core
