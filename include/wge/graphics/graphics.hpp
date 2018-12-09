#pragma once

#include <wge/graphics/render_batch_2d.hpp>
#include <wge/graphics/graphics_backend.hpp>

namespace wge::graphics
{

class graphics
{
public:
	void initialize(window_backend_type pWindowing, backend_type pRendering)
	{
		mWindow_backend = window_backend::create(pWindowing, pRendering);
		mGraphics_backend = graphics_backend::create(pRendering);
		mGraphics_backend->initialize();
	}

	window_backend::ptr get_window_backend() const
	{
		return mWindow_backend;
	}

	graphics_backend::ptr get_graphics_backend() const
	{
		return mGraphics_backend;
	}

private:
	window_backend::ptr mWindow_backend;
	graphics_backend::ptr mGraphics_backend;
};

} // namespace wge::graphics
