#pragma once

#include <wge/graphics/graphics_backend.hpp>
#include <wge/graphics/glfw_backend.hpp>
#include <wge/logging/log.hpp>

#include <thread>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "null_backend.hpp"

namespace wge::graphics
{

glfw_window_backend::glfw_window_backend()
{
	glfwInit();

	// OpenGL 3.3 minimum
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Make Mac happy
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // No old OpenGL

	// Create our window and initialize the opengl context
	mWindow = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
	glfwMakeContextCurrent(mWindow);

	// Match the monitors refresh rate (VSync)
	glfwSwapInterval(1);

	glfwSetWindowUserPointer(mWindow, this);

	glfwSetDropCallback(mWindow, [](GLFWwindow* pWindow, int pCount, const char** pPaths)
	{
		glfw_window_backend* window = static_cast<glfw_window_backend*>(glfwGetWindowUserPointer(pWindow));
		window->on_file_drop(pCount, pPaths);
	});
	glfwSetWindowFocusCallback(mWindow, [](GLFWwindow* pWindow, int pIs_focused)
	{
		glfw_window_backend* window = static_cast<glfw_window_backend*>(glfwGetWindowUserPointer(pWindow));
		window->mLimited_frame_mode = pIs_focused == GLFW_FALSE;
	});
}

glfw_window_backend::~glfw_window_backend()
{
	// Cleanup GLFW
	glfwDestroyWindow(mWindow);
	glfwTerminate();
}

int glfw_window_backend::get_display_width()
{
	int width = 0;
	glfwGetWindowSize(mWindow, &width, NULL);
	return width;
}

int glfw_window_backend::get_display_height()
{
	int height = 0;
	glfwGetWindowSize(mWindow, NULL, &height);
	return height;
}

void glfw_window_backend::refresh()
{
	if (mLimited_frame_mode)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	glfwMakeContextCurrent(mWindow);
	glfwSwapBuffers(mWindow);
}

void glfw_window_backend::serialize_settings(json& pJson)
{
	int width = 0, height = 0;
	glfwGetWindowSize(mWindow, &width, &height);
	pJson["window_width"] = width;
	pJson["window_height"] = height;

	bool maximized = glfwGetWindowAttrib(mWindow, GLFW_MAXIMIZED) == GLFW_TRUE;
	pJson["window_maximized"] = maximized;
}

void glfw_window_backend::deserialize_settings(const json& pJson)
{
	glfwSetWindowSize(mWindow, pJson["window_width"], pJson["window_height"]);
	if (pJson["window_maximized"].get<bool>())
		glfwMaximizeWindow(mWindow);
}

GLFWwindow* glfw_window_backend::get_window() const
{
	return mWindow;
}

window_backend::ptr create_glfw_backend(backend_type pFor_backend)
{
	if (pFor_backend != backend_type::opengl)
	{
		log::error("GLFW window backend only supports the OpenGL rendering backend, default null.");
		return std::make_shared<null_window_backend>();
	}
	return std::make_shared<glfw_window_backend>();
}

} // namespace wge::graphics
