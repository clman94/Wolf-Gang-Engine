#include <wge/graphics/image.hpp>

#include <algorithm>
#include <stb/stb_image.h>

namespace wge::graphics
{

bool image::load_file(const std::string& pPath)
{
	int width = 0, height = 0, channel_count = 0;
	stbi_uc* data = stbi_load(&pPath[0], &width, &height, &channel_count, 4);
	mWidth = static_cast<std::size_t>(width);
	mHeight = static_cast<std::size_t>(height);
	mChannel_count = static_cast<std::size_t>(channel_count);

	std::size_t length = mWidth * mHeight * mChannel_count;
	mData.resize(length);
	std::copy(data, data + length, mData.begin());
	stbi_image_free(data);
	return true;
}

math::vec2 image::get_size() const noexcept
{
	return{ static_cast<float>(mWidth), static_cast<float>(mHeight) };
}

std::size_t image::get_width() const noexcept
{
	return mWidth;
}

std::size_t image::get_height() const noexcept
{
	return mHeight;
}

std::size_t image::get_channel_count() const noexcept
{
	return mChannel_count;
}

util::span<const unsigned char> image::get_raw() const noexcept
{
	return mData;
}

} // namespace wge::graphics
