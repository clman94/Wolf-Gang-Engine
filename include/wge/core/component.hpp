#pragma once

#include <wge/core/messaging.hpp>

#include <nlohmann/json.hpp>
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
	virtual int get_id() const override { return id__; } \
	virtual std::string get_name() const override { return name__; }

namespace wge::core
{

class object_node;

class component :
	protected subscriber
{
protected:
	component(object_node* pObj) :
		mObject(pObj)
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

	virtual std::string get_name() const = 0;
	virtual int get_id() const = 0;

	// Get the object this component is apart of 
	object_node* get_object()
	{
		return mObject;
	}

private:
	object_node* mObject;
	int mInstance_id;
};

}