#include <wge/graphics/image.hpp>

#include <algorithm>
#include <stb/stb_image.h>

namespace wge::graphics
{

bool image::load_file(const std::string& pPath)
{
	stbi_uc* data = stbi_load(&pPath[0], &mWidth, &mHeight, &mChannel_count, 4);
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

int image::get_width() const noexcept
{
	return mWidth;
}

int image::get_height() const noexcept
{
	return mHeight;
}

int image::get_channel_count() const noexcept
{
	return mChannel_count;
}

util::span<const unsigned char> image::get_raw() const noexcept
{
	return mData;
}

} // namespace wge::graphics
