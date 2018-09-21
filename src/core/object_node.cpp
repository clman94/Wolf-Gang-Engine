
#include <wge/core/object_node.hpp>
#include <wge/core/component.hpp>

using namespace wge;
using namespace wge::core;

inline bool starts_with(const std::string& pStr, const std::string& pPrefix)
{
	if (pStr.length() < pPrefix.length())
		return false;
	return std::string_view(pStr).substr(0, pPrefix.length()) == pPrefix;
}

// Generates a uniquely numbered name from a list
template <typename Titer, typename Tgetter>
inline std::string create_unique_name(std::string pPrefix, Titer pBegin, Titer pEnd, Tgetter&& pGetter)
{
	// This is the largest value found
	int max = 0;

	// True if we found an exact match to pPrefix
	bool found_exact = false;

	const std::string prefix_numbered = pPrefix + "_";
	for (; pBegin != pEnd; pBegin++)
	{
		const std::string& str = pGetter(*pBegin);

		// This hasn't found any numbered matches yet,
		// but it found an exact match to the pPrefix.
		if (pPrefix == str && max == 0)
			found_exact = true;

		// Found a match to a numbered value
		else if (starts_with(str, prefix_numbered)
			&& str.length() > prefix_numbered.length())
		{
			// Parse number
			int val = std::atoi(str.substr(prefix_numbered.length()).c_str());

			// Record it if it is larger
			max = val > max ? val : max;
		}
	}

	if (!found_exact)
		return pPrefix;
	else
		return pPrefix + "_" + std::to_string(max + 1);
}

util::ref<object_node> object_node::create()
{
	return util::ref<object_node>::create();
}

object_node::~object_node()
{
	remove_children();
}

bool object_node::has_component(int pId) const
{
	for (auto& i : mComponents)
		if (i->get_component_id() == pId)
			return true;
	return false;
}

component* object_node::get_component(const std::string & pName)
{
	return nullptr;
}

component* object_node::get_component(int pId) const
{
	for (auto& i : mComponents)
		if (i->get_component_id() == pId)
			return i.get();
	return nullptr;
}

void object_node::remove_component(std::size_t pIndex)
{
	mComponents.erase(mComponents.begin() + pIndex);
}

void object_node::remove_components()
{
	mComponents.clear();
}

json object_node::serialize() const
{
	using nlohmann::json;
	json result;
	result["name"] = mName;

	// Serialize the components
	{
		std::vector<json> components_json;
		for (const auto& i : mComponents)
		{
			json c;
			c["id"] = i->get_component_id();
			c["name"] = i->get_name();
			c["data"] = i->serialize();
			components_json.push_back(c);
		}
		result["components"] = components_json;
	}

	// Serialize the children
	{
		std::vector<json> children_json;
		for (const auto& i : mChildren)
			children_json.push_back(i->serialize());
		result["children"] = children_json;
	}

	return result;
}

void object_node::deserialize(const json& pJson, const component_factory& pFactory)
{
	mName = pJson["name"];

	for (const json& i : pJson["components"])
	{
		component* c = pFactory.create(i["id"], this);
		c->set_name(i["name"]);
		c->deserialize(i["data"]);
		mComponents.emplace_back(c);
	}

	for (const json& i : pJson["children"])
	{
		auto obj = object_node::create();
		obj->deserialize(i, pFactory);
		add_child(obj);
	}
}

component* object_node::get_component_index(std::size_t pIndex) const
{
	return mComponents[pIndex].get();
}

std::size_t object_node::get_component_count() const
{
	return mComponents.size();
}

void object_node::set_name(const std::string & pName)
{
	mName = pName;
}

const std::string & object_node::get_name()
{
	return mName;
}

std::size_t object_node::get_child_count() const
{
	return mChildren.size();
}

util::ref<object_node> object_node::get_child(std::size_t pIndex) const
{
	return mChildren[pIndex];
}

util::ref<object_node> object_node::create_child()
{
	auto node = object_node::create();
	add_child(node);
	return node;
}

void object_node::add_child(util::ref<object_node> pNode)
{
	if (pNode == this)
		return;
	pNode->remove_parent();
	pNode->mParent = this;
	mChildren.push_back(pNode);
}

void object_node::add_child(util::ref<object_node> pNode, std::size_t pIndex)
{
	if (pNode == this)
		return;
	pNode->remove_parent();
	pNode->mParent = this;
	mChildren.insert(mChildren.begin() + std::min(pIndex, mChildren.size()), pNode);
}

std::size_t object_node::get_child_index(util::ref<object_node> pNode) const
{
	for (std::size_t i = 0; i < mChildren.size(); i++)
		if (pNode == mChildren[i])
			return i;
	return 0;
}

bool object_node::remove_child(util::ref<object_node> pNode)
{
	for (std::size_t i = 0; i < mChildren.size(); i++)
		if (mChildren[i] == pNode)
		{
			mChildren.erase(mChildren.begin() + i);
			return true;
		}
	return false;
}

bool object_node::remove_child(std::size_t pIndex)
{
	if (pIndex >= mChildren.size())
		return false;
	mChildren[pIndex]->mParent.reset();
	mChildren.erase(mChildren.begin() + pIndex);
	return true;
}

void object_node::remove_children()
{
	for (util::ref<object_node> i : mChildren)
	{
		i->mParent.reset();
		i->send_down("on_parent_removed");
	}
	mChildren.clear();
}

bool object_node::is_child_of(util::ref<object_node> pNode) const
{
	if (pNode == get_parent())
		return true;
	if (mParent)
		return get_parent()->is_child_of(pNode);
	return false;
}

util::ref<object_node> object_node::get_parent() const
{
	return mParent.lock();
}

void object_node::remove_parent()
{
	if (auto parent = mParent.lock())
	{
		for (std::size_t i = 0; i < parent->mChildren.size(); i++)
		{
			if (parent->mChildren[i] == this)
			{
				parent->mChildren.erase(parent->mChildren.begin() + i);
				break; // Bail early
			}
		}
		mParent.reset();
		send_down("on_parent_removed");
	}
}

std::string object_node::get_unique_component_name(std::string pPrefix)
{
	return create_unique_name(pPrefix,
		mComponents.begin(), mComponents.end(),
		[](const auto& i) { return i->get_name(); });
}

util::ref<object_node> wge::core::find_first_parent_with_component(int pId, util::ref<object_node> pNode)
{
	while (pNode = pNode->get_parent())
		if (pNode->has_component(pId))
			return pNode;
	return{};
}
