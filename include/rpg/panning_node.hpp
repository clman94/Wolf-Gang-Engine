#ifndef RPG_PANNING_NODE_HPP
#define RPG_PANNING_NODE_HPP

#include <engine/node.hpp>
#include <engine/vector.hpp>
#include <engine/rect.hpp>

namespace rpg {

/// A node that acts as a sophisticated camera that can focus on a point.
class panning_node :
	public engine::node
{
public:
	panning_node();

	/// Set the region in which the camera will always stay within
	void set_boundary(engine::frect pBoundary);
	engine::frect get_boundary();

	/// Set the camera's resolution
	void set_viewport(engine::fvector pViewport);

	/// Set the focal point in which the camera will center on
	void set_focus(engine::fvector pFocus);
	engine::fvector get_focus();

	void set_boundary_enable(bool pEnable);

private:
	engine::frect mBoundary;
	engine::fvector mViewport;
	engine::fvector mFocus;
	bool mBoundary_enabled;
};

}
#endif // !RPG_PANNING_NODE_HPP
