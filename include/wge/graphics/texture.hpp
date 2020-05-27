#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <optional>

#include <wge/util/json_helpers.hpp>
#include <wge/util/uuid.hpp>
#include <wge/math/rect.hpp>
#include <wge/math/aabb.hpp>
#include <wge/core/asset.hpp>
#include <wge/filesystem/filesystem_interface.hpp>
#include <wge/graphics/image.hpp>

namespace wge::graphics
{

// Interface for the texture implementation.
class texture_impl
{
public:
	using ptr = std::shared_ptr<texture_impl>;
	virtual ~texture_impl() {}
	virtual void create_from_image(const image&) = 0;
	virtual void set_smooth(bool pSmooth) = 0;
	virtual bool is_smooth() const = 0;
};

class texture
{
public:
	void set_implementation(const texture_impl::ptr& pImpl) noexcept;
	texture_impl::ptr get_implementation() const noexcept;

	void set_image(image&& pImage);
	void set_image(const image& pImage);
	const image& get_image() const noexcept { return mImage; }

	// Get width of texture in pixels
	int get_width() const noexcept;

	// Get height of texture in pixels
	int get_height() const noexcept;

	math::ivec2 get_size() const noexcept;

	// Set the smooth filtering. If enabled,
	// the image will get smoothed when stretched or rotated
	// making it more pleasing to the eye. However, it may be
	// a good idea to disable this if your doing pixel art as it
	// tends to "blur" tiny images.
	void set_smooth(bool pEnabled) noexcept;
	bool is_smooth() const noexcept;

private:
	void update_impl_image();

private:
	filesystem::path mPath;
	texture_impl::ptr mImpl;
	bool mSmooth{ false };
	image mImage;
};

} // namespace wge::graphics
