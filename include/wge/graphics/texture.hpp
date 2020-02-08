#pragma once

#include <memory>
#include <string>
#include <iostream>

#include <wge/util/json_helpers.hpp>
#include <wge/util/uuid.hpp>
#include <wge/math/rect.hpp>
#include <wge/core/asset.hpp>
#include <wge/filesystem/filesystem_interface.hpp>
#include <wge/graphics/image.hpp>

namespace wge::graphics
{

struct animation
{
public:
	std::string name;
	std::size_t frames{ 1 };
	float interval{ 0 };
	math::rect frame_rect;
	util::uuid id;

	animation() = default;
	animation(const json& pJson);

	// Parse json
	void deserialize(const json& pJson);
	[[nodiscard]] json serialize() const;
};

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

class texture :
	public core::resource
{
public:
	using atlas_container = std::vector<animation>;
	using handle = core::resource_handle<texture>;

	virtual ~texture() {}

	void set_implementation(const texture_impl::ptr& pImpl) noexcept;
	texture_impl::ptr get_implementation() const noexcept;

	// Load a texture from a file
	virtual void load() override;

	// Get width of texture in pixels
	int get_width() const noexcept;

	// Get height of texture in pixels
	int get_height() const noexcept;

	math::vec2 get_size() const noexcept;

	// Set the smooth filtering. If enabled,
	// the image will get smoothed when stretched or rotated
	// making it more pleasing to the eye. However, it may be
	// a good idea to disable this if your doing pixel art as it
	// tends to "blur" tiny images.
	void set_smooth(bool pEnabled) noexcept;
	bool is_smooth() const noexcept;

	// Retrieve an animation by name. Returns an empty pointer if it was not found.
	animation* get_animation(const std::string& pName) noexcept;
	animation* get_animation(const util::uuid& pId) noexcept;

	// Get the raw container for the atlas.
	// Mainly for use by an editor.
	atlas_container& get_raw_atlas() noexcept;
	const atlas_container& get_raw_atlas() const noexcept;

private:
	virtual json serialize_data() const override;
	virtual void deserialize_data(const json& pJson) override;

private:
	filesystem::path mPath;
	texture_impl::ptr mImpl;
	bool mSmooth{ false };
	image mImage;
	atlas_container mAtlas;
	// util::uuid mDefault_animation; // TODO: Use instead of a "Default" animation
};

} // namespace wge::graphics
