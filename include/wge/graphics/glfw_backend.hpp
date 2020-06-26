#pragma once

#include <wge/graphics/graphics_backend.hpp>
#include <wge/util/json_helpers.hpp>
#include <memory>

struct GLFWwindow;

namespace wge::graphics
{

class glfw_window_backend :
	public window_backend
{
public:
	using ptr = std::shared_ptr<glfw_window_backend>;

	glfw_window_backend();
	virtual ~glfw_window_backend();
	virtual int get_display_width() override;
	virtual int get_display_height() override;
	virtual void refresh() override;
	virtual void serialize_settings(json&) override;
	virtual void deserialize_settings(const json&) override;

	GLFWwindow* get_window() const;

private:
	bool mLimited_frame_mode = false;
	GLFWwindow* mWindow;
};

} // namespace wge::graphics
