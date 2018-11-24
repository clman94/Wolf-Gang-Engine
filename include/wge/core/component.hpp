#pragma once

#include <string>
#include <array>

#include <wge/math/aabb.hpp>
#include <wge/core/messaging.hpp>
#include <wge/core/serializable.hpp>

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

	context& get_context() const;

	virtual bool has_aabb() const { return false; }
	virtual math::aabb get_screen_aabb() const { return{}; };

	int get_object_id() const
	{
		return mObject_id;
	}

	void set_object(int pId)
	{
		mObject_id = pId;
	}

	int get_instance_id()
	{

	}

private:
	std::string mName;
	int mObject_id;
	int mInstance_id;
};

template <typename...T>
class component_requires
{
	template <typename T>
	static constexpr bool requires_component()
	{
		return requires_component(T::COMPONENT_ID);
	}

	static constexpr bool requires_component(int pId)
	{
		bool has = false;
		(has |= T::COMPONENT_ID == pId, ...);
		return has;
	}

	static constexpr std::array<int, sizeof...(T)> get_all_required_components()
	{
		return { T::COMPONENT_ID... };
	}
};

template <typename Trequires>
class component_definition :
	component
{
public:
	virtual ~component() {}

};

} // namespace wge::core
