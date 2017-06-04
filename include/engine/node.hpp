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

	// Absolute position scaled by unit scale
	fvector  get_exact_position() const;

	void     set_absolute_position(const fvector& pPosition);
	fvector  get_absolute_position() const;

	void     set_position(const fvector& pPosition);
	fvector  get_position() const;
	fvector  get_position(const node& pRelative) const;

	util::optional_pointer<node> detach_parent();
	node_arr                     detach_children();

	util::optional_pointer<node> get_parent() const;
	node_arr                     get_children() const;

	bool set_parent(node& obj);
	bool add_child(node& obj);

	void set_unit(float pUnit);
	float get_unit() const;

private:
	util::optional_pointer<node> mParent;
	node_arr mChildren;

	size_t mChild_index;

	fvector mPosition;

	float mUnit;
};

}

#endif