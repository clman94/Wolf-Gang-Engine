#pragma once

#include <wge/math/math.hpp>
#include <wge/math/vector.hpp>
#include <wge/math/aabb.hpp>

namespace wge::graphics
{

class camera
{
public:
	void set_zoom(float pZoom) { /* TODO */ }
	float get_zoom() const { return mZoom; }
	void zoom(float pAmount) { /* TODO */ }

	void set_focus(const math::vec2& pPosition) { /* TODO */ }
	math::vec2 get_focus() const { return mFocus; }
	void move_focus(const math::vec2& pAmount) { /* TODO */ }

	void set_boundary_enable(bool mEnabled) { /* TODO */ }
	bool is_boundary_enabled() const { return mBoundary_enabled; }
	void set_boundary(const math::aabb& pLimit) { /* TODO */ }
	math::aabb get_boundary() const { return mBoundary; }

	void envelop(const math::aabb& pTarget, const math::ivec2& pScreen_size)
	{
		mFocus = math::midpoint(pTarget.min, pTarget.max);
		// TODO
	}

	void envelop(const math::aabb& pTarget)
	{
		mNeeds_envelop = true;
		mEnvelop_target = pTarget;
	}

	math::aabb get_view(const math::ivec2& pScreen_size)
	{
		// TODO
		math::aabb view;
		return view;
	}

private:
	bool mBoundary_enabled = false;
	math::aabb mBoundary;

	bool mNeeds_envelop = false;
	math::aabb mEnvelop_target;

	math::vec2 mFocus;
	float mZoom = 0;
};

}
