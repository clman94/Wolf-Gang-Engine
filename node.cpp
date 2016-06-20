
#include "node.hpp"

using namespace engine;

node::node()
{
	parent.reset();
}

node::~node()
{
	printf("delete\n");
	detach_children();
	detach_parent();
}

void
node::update_position()
{
	if (!parent.is_null())
		position = relative_position + parent->get_position();

	for (auto &i : children)
	{
		i->update_position();
	}
}

fvector
node::get_relative_position()
{
	return relative_position;
}

fvector
node::get_position()
{
	return position;
}

void
node::set_relative_position(fvector pos)
{
	relative_position = pos;
	update_position();
}

void
node::set_position(fvector pos)
{
	if (!parent.is_null())
		relative_position = pos - parent->position;
	else
		position = pos;
	update_position();
}

node_ref
node::detach_parent()
{
	if (parent.is_null()) return nullptr;
	node_ref temp = parent->children[child_index];
	parent->children.erase(parent->children.begin() + child_index);
	for (size_t i = 0; i < parent->children.size(); i++)
	{
		parent->children[i]->child_index = i;
	}
	child_index = -1;
	parent.reset();
	return temp;
}

node_arr
node::detach_children()
{
	if (!children.size()) return node_arr();
	node_arr temp = children;
	for (auto i : children)
	{
		i->parent.reset();
	}
	children.clear();
	return temp;
}

node_ref
node::get_parent()
{
	return parent;
}

node_arr
node::get_children()
{
	return children;
}

int
node::set_parent(node_ref obj)
{
	return obj->add_child(*this);
}

int
node::add_child(node_ref obj)
{
	if (!obj->parent.is_null()) obj->detach_parent();
	obj->child_index = children.size();
	obj->parent = *this;
	children.push_back(obj);
	return 0;
}
