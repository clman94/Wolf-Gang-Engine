#pragma once

#include <memory>
#include <string>
#include <iostream>

#include <nlohmann/json.hpp>
#include <wge/util/json_helpers.hpp>
using json = nlohmann::json;

#include <wge/math/rect.hpp>
#include <wge/core/asset.hpp>
#include <wge/filesystem/filesystem_interface.hpp>

namespace wge::graphics
{

struct animation
{
public:
	using ptr = std::shared_ptr<animation>;

	std::string name;
	std::size_t frames{ 1 };
	float interval{ 0 };
	math::rect frame_rect;

	animation() = default;
	animation(const json& pJson);

	// Parse json
	void load(const json& pJson);
	[[nodiscard]] json save() const;
};

class texture_impl
{
public:
	using ptr = std::shared_ptr<texture_impl>;
	virtual ~texture_impl() {}
	virtual void create_from_pixels(unsigned char* pBuffer, int pWidth, int pHeight, int mChannels) = 0;
	virtual void set_smooth(bool pSmooth) = 0;
	virtual bool is_smooth() const = 0;
};

class texture :
	public core::asset
{
public:
	using atlas_container = std::vector<animation::ptr>;
	using ptr = tptr<texture>;

	texture(core::asset_config::ptr pConfig);
	~texture();

	void set_implementation(const texture_impl::ptr& pImpl);
	texture_impl::ptr get_implementation() const;

	// Load a texture from a file
	void load(const std::string& pFilepath);

	// Load texture from a stream. If pSize = 0, the rest of the stream will be used.
	void load(filesystem::stream::ptr pStream, std::size_t pSize = 0);

	// Get width of texture in pixels
	int get_width() const;

	// Get height of texture in pixels
	int get_height() const;

	// Set the smooth filtering. If enabled,
	// the image will get smoothed when stretched or rotated
	// making it more pleasing to the eye. However, it may be
	// a good idea to disable this if your doing pixel art as it
	// tends to "blur" tiny images.
	void set_smooth(bool pEnabled);
	bool is_smooth() const;

	// Retrieve an animation by name. Returns an empty pointer if it was not found.
	animation::ptr get_animation(const std::string& pName) const;

	// Get the raw container for the atlas.
	// Mainly for use by an editor.
	atlas_container& get_raw_atlas();
	const atlas_container& get_raw_atlas() const;

private:
	// Update the configuration with the current atlas
	void update_metadata() const;
	// Load the atlas from the metadata
	void load_metadata();

private:
	texture_impl::ptr mImpl;
	int mWidth, mHeight, mChannels;
	bool mSmooth;
	unsigned char* mPixels;
	atlas_container mAtlas;
};

} // namespace wge::graphics