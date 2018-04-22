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

	void set_absolute_position(const fvector& pPosition);
	void set_position(const fvector& pPosition);
	void set_rotation(float pRotation);
	void set_scale(fvector pScale);

	fvector get_position() const;
	fvector get_position(const node& pRelative) const;
	float   get_rotation() const;
	fvector get_scale() const;

	fvector get_absolute_position() const;
	float   get_absolute_rotation() const;
	fvector get_absolute_scale() const;

	// This node is will not be visible to the parent node
	void set_internal_parent(node& pNode);

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
	bool mIs_internal;
	node_arr mChildren;

	size_t mChild_index;

	float mUnit;

	fvector mPosition;
	float mRotation;
	fvector mScale;
};

// Calculates exact position of point relative to node
fvector exact_relative_to_node(fvector pPosition, const node & pNode);

}

#endif