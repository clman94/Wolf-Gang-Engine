#ifndef NODE_HPP
#define NODE_HPP

#include <memory>
#include <vector>
#include "vector.hpp"
#include "utility.hpp"

namespace engine
{
 
class node;

typedef std::vector<node*> node_arr;

// Provides the backbone of objects and 
// their representation in game space
class node
{
public:
	node();

	virtual ~node();

	void     set_exact_position(fvector pos);
	fvector  get_exact_position() const;

	void     set_position(fvector pos);
	fvector  get_position() const;
	fvector  get_position(const node& pRelative) const;

	node*    detach_parent();
	node_arr detach_children();

	node*    get_parent();
	node_arr get_children();

	int      set_parent(node& obj);
	int      add_child(node& obj);

private:
	node* mParent;
	node_arr mChildren;

	size_t mChild_index;

	fvector mPosition;
};

}

#endif