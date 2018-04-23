
#include <engine/node.hpp>

using namespace engine;

inline bool check_node_loop(const node& pFind, const node& pStart)
{
	auto curr_node = &pStart;
	while (curr_node)
	{
		if (curr_node == &pFind)
			return true;
		if (!curr_node->get_parent())
			return false;
		curr_node = curr_node->get_parent();
	}
	return false;
}

node::node()
{
	mIs_internal = false;
	mParent = nullptr;
	mUnit = 1;
	mRotation = 0;
	mScale = { 1, 1 };
}

node::~node()
{
	//printf("delete\n");
	detach_children();
	detach_parent();
}

fvector node::get_exact_position() const
{
	return get_absolute_position()*mUnit;
}

fvector node::get_absolute_position() const
{
	if (!mParent) return mPosition;
	return (mPosition*get_absolute_scale()).rotate(mParent->get_absolute_rotation())
		+ mParent->get_absolute_position();
}

fvector node::get_position() const
{
	return mPosition;
}

fvector node::get_position(const node& pRelative) const
{
	return get_absolute_position() - pRelative.get_absolute_position();
}

void node::set_rotation(float pRotation)
{
	mRotation = pRotation;
}

void node::set_scale(fvector pScale)
{
	mScale = pScale;
}

float node::get_rotation() const
{
	return mRotation;
}

fvector node::get_scale() const
{
	return mScale;
}

float node::get_absolute_rotation() const
{
	if (!mParent) return mRotation;
	return mParent->get_absolute_rotation() + mRotation;
}

fvector node::get_absolute_scale() const
{
	if (!mParent) return mScale;
	return mParent->get_absolute_scale()*mScale;
}

void node::set_internal_parent(node & pNode)
{
	detach_parent();
	mParent = &pNode;
	mIs_internal = true;
}

void node::set_absolute_position(const fvector& pPosition)
{
	if (mParent)
		mPosition = pPosition - mParent->get_absolute_position();
	else
		mPosition = pPosition;
}

void node::set_position(const fvector& pPosition)
{
	mPosition = pPosition;
}

util::optional_pointer<node> node::detach_parent()
{
	if (!mParent) return{};
	if (mIs_internal)
	{
		node* temp = mParent;
		mParent.reset();
		mIs_internal = false;
		return temp;
	}
	node* temp = mParent->mChildren[mChild_index];
	mParent->mChildren.erase(mParent->mChildren.begin() + mChild_index);
	for (size_t i = 0; i < mParent->mChildren.size(); i++)
		mParent->mChildren[i]->mChild_index = i;
	mChild_index = -1;
	mParent.reset();
	return temp;
}

node_arr node::detach_children()
{
	if (!mChildren.size()) return node_arr();
	node_arr temp = mChildren;
	for (auto i : mChildren)
		i->mParent.reset();
	mChildren.clear();
	return temp;
}

util::optional_pointer<node> node::get_parent() const
{
	return mParent;
}

node_arr node::get_children() const
{
	return mChildren;
}

bool node::set_parent(node& obj)
{
	return obj.add_child(*this);
}

bool node::add_child(node& obj)
{
	if (&obj == this) return false;
	if (obj.mParent) obj.detach_parent();
	obj.mChild_index = mChildren.size();
	obj.mParent = this;
	obj.set_unit(mUnit);
	mChildren.push_back(&obj);
	return true;
}

void node::set_unit(float pUnit)
{
	assert(pUnit > 0);
	mUnit = pUnit;

	// Set unit of children
	for (auto i : mChildren)
		i->set_unit(pUnit);
}

float node::get_unit() const
{
	return mUnit;
}

fvector engine::exact_relative_to_node(fvector pPosition, const node & pNode)
{
	return ((pPosition*pNode.get_absolute_scale()).rotate(pNode.get_absolute_rotation())
		+ pNode.get_absolute_position())*pNode.get_unit();
}

frect engine::exact_relative_to_node(frect pRect, const node & pNode)
{
	return{
		exact_relative_to_node(pRect.get_offset(), pNode),
		pRect.get_size()*pNode.get_absolute_scale()*pNode.get_unit()
	};
}
