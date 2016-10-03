#ifndef NODE_HPP
#define NODE_HPP

#include <memory>
#include <vector>
#include "vector.hpp"
#include "ptr_GC.hpp"

namespace engine
{
 
class node;

typedef ptr_GC<node> node_ref;
typedef std::vector<node_ref> node_arr;

// Provides the backbone of objects and 
// their representation in game space
class node
{
	node_ref mParent;
	node_arr mChildren;

	size_t mChild_index; 

	fvector mPosition;

public:

	node();

	virtual ~node();

	void     set_exact_position(fvector pos);
	fvector  get_exact_position() const;

	void     set_position(fvector pos);
	fvector  get_position() const;
	fvector  get_position(const node& pRelative) const;

	node_ref detach_parent();
	node_arr detach_children();

	node_ref get_parent();
	node_arr get_children();

	int set_parent(node_ref obj);
	int add_child(node_ref obj);
};

}

#endif