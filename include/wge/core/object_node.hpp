#pragma once

#include <string>
#include <vector>
#include <memory>

#include <wge/util/ref.hpp>
#include <wge/core/messaging.hpp>

#include <nlohmann/json.hpp>
using nlohmann::json;

namespace wge::core
{

// This template is used to check if a type has the "COMPONENT_ID" member.
template <typename T, typename = int>
struct has_component_id_member : std::false_type {};
template <typename T>
struct has_component_id_member <T, decltype((void)T::COMPONENT_ID, 0)> : std::true_type {};

class component;
class object_node;

class component_factory
{
public:
	component* create(int pId, object_node* pNode) const
	{
		if (mFactories.find(pId) == mFactories.end())
			return nullptr;
		return (*mFactories.find(pId)).second(pNode);
	}

	template <class T>
	void add()
	{
		mFactories[T::COMPONENT_ID] = [](object_node* pNode)->component*
		{
			return new T(pNode);
		};
	}

private:
	std::map<int, std::function<component*(object_node*)>> mFactories;
};

// Object nodes are game objects that relay
// messages between components and to other 
// game objects.
class object_node :
	public util::ref_counted,
	public publisher
{
public:
	static util::ref<object_node> create();

public:
	~object_node();

	// Creates a component and adds it to this object
	template<class T,
		// T has to derive the component class and requires the "int COMPONENT_ID" member
		typename = std::enable_if<std::is_base_of<component, T>::value && has_component_id_member<T>::value>::type>
	T* add_component()
	{
		if (has_component<T>())
			return nullptr;
		T* ptr = new T(this);
		mComponents.push_back(std::unique_ptr<component>(static_cast<component*>(ptr)));
		return ptr;
	}

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

	// Get component by id
	component* get_component(int pId) const;
	// Get component by type
	template<class T,
		// Requires the "int COMPONENT_ID" COMPONENT_ID member
		typename = std::enable_if<has_component_id_member<T>::value>::type>
	T* get_component() const
	{
		return static_cast<T*>(get_component(T::COMPONENT_ID));
	}

	void remove_components();
	
	// Serialize this node. This will include all children nodes.
	json serialize() const;

	void deserialize(const json& pJson, const component_factory& pFactory);

	// Get component by index
	component* get_component_index(std::size_t pIndex) const;
	// Get total count of components connected to this object
	std::size_t get_component_count() const;

	// Set the name of this object
	void set_name(const std::string& pName);
	// Get the name of this object
	const std::string& get_name();

	// Creates the child object
	util::ref<object_node> create_child();
	// Get total children connected to this object
	std::size_t get_child_count() const;
	// Get child by index
	util::ref<object_node> get_child(std::size_t pIndex) const;

	// Add a child object
	void add_child(util::ref<object_node> pNode);
	// Remove a child object by reference. Returns true if successful.
	bool remove_child(util::ref<object_node> pNode);
	// Remove a child object by index. Returns true if successful.
	bool remove_child(std::size_t pIndex);
	// Remove all children
	void remove_children();

	// Get parent object
	util::ref<object_node> get_parent();
	// Detach the parent object
	void remove_parent();

	// Broadcast an event from this object and up through each parent
	template<class...Targs>
	void send_up(const std::string& pEvent_name, Targs...pArgs)
	{
		send(pEvent_name, pArgs...);
		if (auto parent = mParent.lock())
			parent->send_up(pEvent_name, pArgs...);
	}

	// Broadcast an event from the top-most parent to the current node.
	// This is useful in situations where the top-most parent node needs
	// to take precedence over its children nodes e.g. the physics components.
	template<class...Targs>
	void send_from_top(const std::string& pEvent_name, Targs...pArgs)
	{
		if (auto parent = mParent.lock())
			parent->send_from_top(pEvent_name, pArgs...);
		send(pEvent_name, pArgs...);
	}

	// Sends a message to this object and to all its children/sub-children
	template<class...Targs>
	void send_down(const std::string& pEvent_name, Targs...pArgs)
	{
		send(pEvent_name, pArgs...);
		for (auto& i : mChildren)
			i->send_down(pEvent_name, pArgs...);
	}

private:
	std::vector<std::unique_ptr<component>> mComponents;

	util::weak_ref<object_node> mParent;
	std::vector<util::ref<object_node>> mChildren;

	std::string mName;
};

util::ref<object_node> find_first_parent_with_component(int pId, util::ref<object_node> pNode);

}