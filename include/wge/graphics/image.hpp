#pragma once

#include <wge/math/vector.hpp>

#include <string_view>
#include <vector>

namespace wge::graphics
{

class image
{
public:
	~image();

	bool load_file(const std::string& pPath);

	// Get vec2 of the size of this image in pixels.
	math::vec2 get_size() const noexcept;

	// Get width of this image in pixels.
	int get_width() const noexcept;

	// Get height of this image in pixels.
	int get_height() const noexcept;

	// Get the amount of channels in this image.
	int get_channel_count() const noexcept;

	const unsigned char* get_raw() const noexcept;

	bool is_valid() const noexcept
	{
		return mData != nullptr;
	}


private:
	int mWidth{ 0 }, mHeight{ 0 }, mChannel_count{ 0 };
	unsigned char* mData{ nullptr };

};

} // namespace wge::graphics
