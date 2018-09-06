
#include <wge/core/object_node.hpp>
#include <wge/core/component.hpp>

using namespace wge;
using namespace wge::core;

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
		if (i->get_id() == pId)
			return true;
	return false;
}

component* object_node::get_component(int pId) const
{
	for (auto& i : mComponents)
		if (i->get_id() == pId)
			return i.get();
	return nullptr;
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
	if (pNode->mParent != this)
	{
		pNode->remove_parent();
		pNode->mParent = this;
		mChildren.push_back(pNode);
	}
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
		i->mParent.reset();
	mChildren.clear();
}

util::ref<object_node> object_node::get_parent()
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
	}
}

util::ref<object_node> wge::core::find_first_parent_with_component(int pId, util::ref<object_node> pNode)
{
	while (pNode = pNode->get_parent())
		if (pNode->has_component(pId))
			return pNode;
	return{};
}