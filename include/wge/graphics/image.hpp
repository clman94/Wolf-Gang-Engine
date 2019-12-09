#pragma once

#include <wge/math/vector.hpp>
#include <wge/graphics/color.hpp>
#include <wge/util/span.hpp>

#include <string_view>
#include <vector>

namespace wge::graphics
{

class image
{
public:
	bool load_file(const std::string& pPath);

	// Get vec2 of the size of this image in pixels.
	math::vec2 get_size() const noexcept;

	// Get width of this image in pixels.
	std::size_t get_width() const noexcept;

	// Get height of this image in pixels.
	std::size_t get_height() const noexcept;

	// Get the amount of channels in this image.
	std::size_t get_channel_count() const noexcept;

	void resize(std::size_t w, std::size_t h)
	{
		mData.reserve(w * h * mChannel_count);
	}

	bool empty() const noexcept
	{
		return mData.empty();
	}

	color get_pixel(std::size_t x, std::size_t y) const
	{
		assert(x < mWidth);
		assert(y < mHeight);
		assert(mChannel_count == 4);
		return color{
			static_cast<float>(mData.at((x + y * mWidth) * mChannel_count)) / 255.f,
			static_cast<float>(mData.at((x + y * mWidth + 1) * mChannel_count)) / 255.f,
			static_cast<float>(mData.at((x + y * mWidth + 2) * mChannel_count)) / 255.f,
			static_cast<float>(mData.at((x + y * mWidth + 3) * mChannel_count)) / 255.f
		};
	}

	util::span<const unsigned char> get_raw() const noexcept;

private:
	std::size_t mWidth{ 0 }, mHeight{ 0 }, mChannel_count{ 0 };
	std::vector<unsigned char> mData;

};

} // namespace wge::graphics
