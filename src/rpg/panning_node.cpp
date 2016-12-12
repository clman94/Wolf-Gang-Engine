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
}

void
panning_node::set_viewport(engine::fvector pViewport)
{
	mViewport = pViewport;
}

void
panning_node::set_focus(engine::fvector pFocus)
{
	mFocus = pFocus;

	if (!mBoundary_enabled)
	{
		set_position(-(pFocus - (mViewport * 0.5f)));
		return;
	}

	engine::fvector offset = pFocus - (mViewport * 0.5f) - mBoundary.get_offset();
	if (mBoundary.w < mViewport.x)
		offset.x = (mBoundary.w * 0.5f) - (mViewport.x * 0.5f);
	else
		offset.x = util::clamp(offset.x, 0.f, mBoundary.w - mViewport.x);

	if (mBoundary.h < mViewport.y)
		offset.y = (mBoundary.h * 0.5f) - (mViewport.y * 0.5f);
	else
		offset.y = util::clamp(offset.y, 0.f, mBoundary.h - mViewport.y);

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