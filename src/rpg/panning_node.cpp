#include <rpg/panning_node.hpp>

using namespace rpg;

panning_node::panning_node()
{
	mBoundary_enabled = true;
}

void
panning_node::set_boundary(engine::frect pBoundary)
{
	mBoundary = pBoundary;
	set_focus(mFocus); // Update focus
}

void
panning_node::set_viewport(engine::fvector pViewport)
{
	mViewport = pViewport;
	set_focus(mFocus); // Update focus
}

void
panning_node::set_focus(engine::fvector pFocus)
{
	mFocus = pFocus;

	// Convert viewport coords to ingame coords
	const engine::fvector viewport = mViewport / get_unit();

	if (!mBoundary_enabled)
	{
		set_position(-(pFocus - (viewport * 0.5f)));
		return;
	}

	engine::fvector offset = pFocus - (viewport * 0.5f) - mBoundary.get_offset();

	if (mBoundary.w < viewport.x)
		offset.x = (mBoundary.w * 0.5f) - (viewport.x * 0.5f);
	else
		offset.x = util::clamp(offset.x, 0.f, mBoundary.w - viewport.x);

	if (mBoundary.h < viewport.y)
		offset.y = (mBoundary.h * 0.5f) - (viewport.y * 0.5f);
	else
		offset.y = util::clamp(offset.y, 0.f, mBoundary.h - viewport.y);

	set_position(-(offset + mBoundary.get_offset()));
}

engine::frect panning_node::get_boundary()
{
	return mBoundary;
}

engine::fvector panning_node::get_focus()
{
	return mFocus;
}

void panning_node::set_boundary_enable(bool pEnable)
{
	mBoundary_enabled = pEnable;
}