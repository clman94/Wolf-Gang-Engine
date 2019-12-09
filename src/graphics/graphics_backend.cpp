#include <wge/graphics/graphics_backend.hpp>
#include <wge/logging/log.hpp>

#include "null_backend.hpp"

namespace wge::graphics
{

graphics_backend::ptr create_opengl_backend();
window_backend::ptr create_glfw_backend(backend_type pFor_backend);

graphics_backend::ptr graphics_backend::create(backend_type pBackend)
{
	switch (pBackend)
	{
	case backend_type::null:
		return std::make_shared<null_graphics_backend>();
	case backend_type::opengl:
		return create_opengl_backend();
	default:
		log::error("Unsupported rendering backend, defaulting with null device.");
		return std::make_shared<null_graphics_backend>();
	}
}

window_backend::ptr window_backend::create(window_backend_type pType, backend_type pFor_backend)
{
	switch (pType)
	{
	case window_backend_type::null:
		return std::make_shared<null_window_backend>();
	case window_backend_type::glfw:
		return create_glfw_backend(pFor_backend);
	default:
		log::error("Unsupported window backend, defaulting with null device.");
		return std::make_shared<null_window_backend>();
	}
}

} // namespace wge::graphics
