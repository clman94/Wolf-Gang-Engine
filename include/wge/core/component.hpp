#pragma once

#include <string>

#include <wge/math/aabb.hpp>
#include <wge/core/messaging.hpp>

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
class object_node;
class asset_manager;

class component :
	protected subscriber
{
protected:
	component(object_node* pNode) :
		mObject(pNode)
	{}

public:
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

	// Get the object this component is apart of 
	object_node* get_object() const
	{
		return mObject;
	}

	context& get_context() const;

	virtual bool has_aabb() const { return false; }
	virtual math::aabb get_screen_aabb() const { return{}; };

protected:
	// Tells the object that this component requires another component to function
	// correctly eg a sprite requires a transform.
	// If the object does not have the component, it will be create automatically.
	template <typename T>
	T* require() const
	{
		assert(mObject);
		if (!mObject->has_component<T>())
			return mObject->add_component<T>();
		return mObject->get_component<T>();
	}

	template <typename T>
	T* get_system() const
	{
		return get_object()->get_context()->get_system<T>();
	}

	// Helper method to get the asset manager from the current context.
	asset_manager* get_asset_manager() const;

private:
	std::string mName;
	object_node* mObject;
	int mInstance_id;
};

} // namespace wge::core
