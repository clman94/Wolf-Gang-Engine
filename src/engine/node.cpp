
#include <engine/node.hpp>

using namespace engine;

node::node()
{
	mParent = nullptr;
}

node::~node()
{
	//printf("delete\n");
	detach_children();
	detach_parent();
}

fvector
node::get_exact_position() const
{
	if (!mParent) return mPosition;
	return mPosition + mParent->get_exact_position();
}

fvector
node::get_position() const
{
	return mPosition;
}

fvector node::get_position(const node& pRelative) const
{
	return get_exact_position() - pRelative.get_exact_position();
}

void
node::set_exact_position(fvector pos)
{
	if (mParent)
		mPosition = pos - mParent->get_exact_position();
	else
		mPosition = pos;
}

void
node::set_position(fvector pos)
{
	mPosition = pos;
}

util::optional_pointer<node>
node::detach_parent()
{
	if (!mParent) return{};
	node* temp = mParent->mChildren[mChild_index];
	mParent->mChildren.erase(mParent->mChildren.begin() + mChild_index);
	for (size_t i = 0; i < mParent->mChildren.size(); i++)
		mParent->mChildren[i]->mChild_index = i;
	mChild_index = -1;
	mParent.reset();
	return temp;
}

node_arr
node::detach_children()
{
	if (!mChildren.size()) return node_arr();
	node_arr temp = mChildren;
	for (auto i : mChildren)
		i->mParent.reset();
	mChildren.clear();
	return temp;
}

util::optional_pointer<node>
node::get_parent()
{
	return mParent;
}

node_arr
node::get_children()
{
	return mChildren;
}

int
node::set_parent(node& obj)
{
	return obj.add_child(*this);
}

int
node::add_child(node& obj)
{
	if (obj.mParent) obj.detach_parent();
	obj.mChild_index = mChildren.size();
	obj.mParent = this;
	mChildren.push_back(&obj);
	return 0;
}
