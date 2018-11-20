#pragma once

#include <string>
#include <vector>
#include <memory>

#include <wge/util/ref.hpp>
#include <wge/core/messaging.hpp>
#include <wge/core/asset_config.hpp>

#include <nlohmann/json.hpp>
#include <wge/util/json_helpers.hpp>
using nlohmann::json;

#include <wge/core/serializable.hpp>

namespace wge::core
{

// This template is used to check if a type has the "COMPONENT_ID" member.
template <typename T, typename = int>
struct has_component_id_member : std::false_type {};
template <typename T>
struct has_component_id_member <T, decltype((void)T::COMPONENT_ID, 0)> : std::true_type {};

class context;
class component;
class object_node;

// Registers factories for components of different types. This is used
// by the object_node class to create components in the deserializing process.
class component_factory
{
public:
	component* create(int pId, object_node* pNode) const
	{
		if (mFactories.find(pId) == mFactories.end())
			return nullptr;
		return (*mFactories.find(pId)).second(pNode);
	}

	template <class T,
		// Requires the "int COMPONENT_ID" member
		typename = std::enable_if<has_component_id_member<T>::value>::type>
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

// Object nodes are objects that relay
// messages between components and to other 
// game objects.
class object_node :
	public util::ref_counted,
	public publisher,
	public serializable
{
public:
	static util::ref<object_node> create(context&);

public:
	using ref = util::ref<object_node>;
	using weak_ref = util::weak_ref<object_node>;

	object_node(context&);
	~object_node();

	// Creates a component and adds it to this object
	template<class T,
		// T has to derive the component class and requires the "int COMPONENT_ID" member
		typename = std::enable_if<std::is_base_of<component, T>::value && has_component_id_member<T>::value>::type>
	T* add_component()
	{
		if (T::COMPONENT_SINGLE_INSTANCE && has_component<T>())
			return nullptr;
		T* ptr = new T(this);
		ptr->set_name(get_unique_component_name(ptr->get_component_name()));
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

	// Get component by name
	component* get_component(const std::string& pName);
	// Get first component by id
	component* get_component(int pId) const;
	// Get component by index
	component* get_component_index(std::size_t pIndex) const;
	// Get first component by type
	template<class T,
		// Requires the "int COMPONENT_ID" COMPONENT_ID member
		typename = std::enable_if<has_component_id_member<T>::value>::type>
	T* get_component() const
	{
		return static_cast<T*>(get_component(T::COMPONENT_ID));
	}
	// Remove component at index
	void remove_component(std::size_t pIndex);
	// Remove all components
	void remove_components();
	// Get total count of components connected to this object
	std::size_t get_component_count() const;
	
	// Serialize this node. This will include all children nodes.
	json serialize() const;
	// Deserialize to this node. This will create all children nodes needed.
	// Since objects don't know about other components, you have to pass in a factory
	// to create those components for it.
	void deserialize(const json& pJson);

	// Set the name of this object
	void set_name(const std::string& pName);
	// Get the name of this object
	const std::string& get_name();

	// Creates the child object
	util::ref<object_node> create_child();
	// Creates the child object with a name
	util::ref<object_node> create_child(const std::string& pName);
	// Get total children connected to this object
	std::size_t get_child_count() const;
	// Get child by index
	util::ref<object_node> get_child(std::size_t pIndex) const;

	// Add a child object
	void add_child(util::ref<object_node> pNode);
	// Insert a child at a position
	void add_child(ref pNode, std::size_t pIndex);
	std::size_t get_child_index(ref pNode) const;
	// Remove a child object by reference. Returns true if successful.
	bool remove_child(ref pNode);
	// Remove a child object by index. Returns true if successful.
	bool remove_child(std::size_t pIndex);
	// Remove all children
	void remove_children();

	bool is_child_of(util::ref<object_node> pNode) const;

	// Get parent object
	util::ref<object_node> get_parent() const;
	// Detach the parent object
	void remove_parent();

	// Sends a message to this object and to all its children/sub-children
	template<class...Targs>
	void send_down(const std::string& pEvent_name, Targs...pArgs)
	{
		send(pEvent_name, pArgs...);
		for (auto& i : mChildren)
			i->send_down(pEvent_name, pArgs...);
	}

	context& get_context() const;

	// Set the id of the asset this object mirrors.
	void set_asset_id(asset_uid pId);
	// Get the id of the asset this object mirrors
	asset_uid get_asset_id() const;
	// Check if this object mirrors serialized object data.
	// If true, the asset data will be deserialized into an object
	// including its children.
	bool has_asset_id() const;

private:
	std::string get_unique_component_name(std::string pPrefix);

private:
	std::vector<std::unique_ptr<component>> mComponents;

	weak_ref mParent;
	std::vector<ref> mChildren;

	asset_uid mAsset_id{ 0 };

	std::string mName;

	std::reference_wrapper<context> mContext;
};

object_node::ref find_first_parent_with_component(int pId, object_node::ref pNode);

} // namespace wge::core
