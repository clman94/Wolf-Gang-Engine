#pragma once

#include <wge/graphics/graphics_backend.hpp>
#include <wge/logging/log.hpp>

namespace wge::graphics
{

class null_framebuffer :
	public framebuffer
{
public:
	virtual void resize(int pWidth, int pHeight) override
	{
	}

	virtual void clear(const color & pColor = { 0,0,0,1 }) override
	{
	}

	virtual int get_width() const override
	{
		return 0;
	}

	virtual int get_height() const override
	{
		return 0;
	}
};

class null_graphics_backend :
	public graphics_backend
{
public:
	virtual void initialize() override
	{
	}

	virtual void render_batch(const framebuffer::ptr & mFramebuffer, const math::mat44 & pProjection, const render_batch_2d & pBatch) override
	{
	}

	virtual framebuffer::ptr create_framebuffer() override
	{
		return std::make_shared<null_framebuffer>();
	}

	virtual texture_impl::ptr create_texture_impl() override
	{
		return {};
	}
};

class null_window_backend :
	public window_backend
{
public:

	virtual int get_display_width() override
	{
		return 0;
	}

	virtual int get_display_height() override
	{
		return 0;
	}
	virtual void refresh() override
	{
	}
};

} // namespace wge::graphics
