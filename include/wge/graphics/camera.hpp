#pragma once

#include <wge/math/math.hpp>
#include <wge/math/vector.hpp>
#include <wge/math/aabb.hpp>

namespace wge::graphics
{

class camera
{
public:
	void set_zoom(float pZoom) { mZoom = pZoom; }
	float get_zoom() const { return mZoom; }
	void zoom(float pAmount) { mZoom += pAmount;  }

	// Set the size of the camera.
	void set_size(const math::vec2& pSize)
	{
		mSize = pSize;
	}
	math::vec2 get_size() const noexcept
	{
		return mSize;
	}

	void set_focus(const math::vec2& pPosition) { mFocus = pPosition; }
	math::vec2 get_focus() const { return mFocus; }
	void move_focus(const math::vec2& pAmount) { mFocus += pAmount; }

	math::aabb get_view() const noexcept
	{
		const float zoom_scale = std::pow(2.f, mZoom);
		const math::vec2 hsize = (mSize / 2.f) * zoom_scale;
		math::aabb view;
		view.min = mFocus - hsize * zoom_scale;
		view.max = mFocus + hsize * zoom_scale;
		return view;
	}

private:
	math::vec2 mSize;
	math::vec2 mFocus;
	float mZoom = 0;
};

}
