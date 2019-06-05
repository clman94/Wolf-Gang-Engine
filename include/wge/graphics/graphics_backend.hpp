#pragma once

#include <wge/graphics/render_batch_2d.hpp>
#include <wge/graphics/texture.hpp>
#include <wge/graphics/framebuffer.hpp>
#include <wge/math/matrix.hpp>
#include <wge/util/signal.hpp>

#include <memory>
#include <functional>

namespace wge::graphics
{

enum class window_backend_type
{
	null,
	glfw
};

enum class backend_type
{
	null,
	opengl
};

class window_backend
{
public:
	using ptr = std::shared_ptr<window_backend>;
	static ptr create(window_backend_type pType, backend_type pFor_backend);

	using file_drop_callback = std::function<void(const std::string&)>;

	virtual ~window_backend() {}
	virtual int get_display_width() = 0;
	virtual int get_display_height() = 0;
	virtual void refresh() = 0;

	util::signal<void()> on_close;
	util::signal<void(int, const char**)> on_file_drop;
};

class graphics_backend
{
public:
	using ptr = std::shared_ptr<graphics_backend>;
	static ptr create(backend_type pBackend);

	virtual ~graphics_backend() {}
	virtual void initialize() = 0;
	virtual void render_batch(const framebuffer::ptr& mFramebuffer, const math::mat44& pProjection, const render_batch_2d& pBatch) = 0;
	virtual framebuffer::ptr create_framebuffer() = 0;
	virtual texture_impl::ptr create_texture_impl() = 0;
};

} // namespace wge::graphics
