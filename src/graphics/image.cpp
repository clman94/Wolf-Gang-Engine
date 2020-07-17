#include <wge/graphics/image.hpp>

#include <algorithm>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace wge::graphics
{

bool image::save_png(const std::string& pPath) const
{
	int r = stbi_write_png(pPath.c_str(), mSize.x, mSize.y, 4, mData.data(), mSize.x * static_cast<int>(channel_count));
	return r == 1;
}

bool image::load_file(const std::string& pPath)
{
	int width = 0, height = 0;
	stbi_uc* data = stbi_load(&pPath[0], &mSize.x, &mSize.y, nullptr, 4);
	if (data == nullptr)
	{
		mError_msg = stbi_failure_reason();
		return false;
	}
	const std::size_t length = static_cast<std::size_t>(mSize.x) * static_cast<std::size_t>(mSize.y) * channel_count;
	mData.resize(length);
	std::copy(data, data + length, mData.begin());
	stbi_image_free(data);
	mError_msg = "";
	return true;
}

math::ivec2 image::get_size() const noexcept
{
	return mSize;
}

int image::get_width() const noexcept
{
	return mSize.x;
}

int image::get_height() const noexcept
{
	return mSize.y;
}

void image::splice(const image& pSrc,
	const math::ivec2& pDest,
	const math::ivec2& pSrc_min,
	const math::ivec2& pSrc_max) noexcept
{
	// Overlapping might occur if the same object is used as the source.
	// The correct alternative is to crop out the section you want
	// to copy and splice it to this image.
	assert(&pSrc != this);

	const math::ivec2 src_size = (pSrc_max - pSrc_min) + math::ivec2{ 1, 1 };
	if (empty() || pSrc.empty() || // Nothing to paste.
		pDest.x >= mSize.x || pDest.y >= mSize.y || // Source is being pasted outside this image.
		pDest.x + src_size.x <= 0 || pDest.y + src_size.y <= 0)
		return; // Just do nothing.

	// The aabb in the destination in which the source will be pasted.
	const math::ivec2 dest_min = math::clamp_components(pDest, math::ivec2{ 0, 0 }, get_size() - math::ivec2{ 1, 1 });
	const math::ivec2 dest_max = math::clamp_components(pDest + (pSrc_max - pSrc_min), math::ivec2{ 0, 0 }, get_size() - math::ivec2{ 1, 1 });

	// Copy the pixels.
	for (int y = dest_min.y; y <= dest_max.y; y++)
		for (int x = dest_min.x; x <= dest_max.x; x++)
			set_pixel({ x, y }, pSrc.get_pixel((math::ivec2{ x, y } - pDest) + pSrc_min));
}

image image::crop(const math::ivec2& pMin, const math::ivec2& pMax, const color8& pOverflow_color) const
{
	assert(pMax.x > pMin.x);
	assert(pMax.y > pMin.y);
	const math::ivec2 new_size = pMax - pMin + math::ivec2{ 1, 1 };
	image new_image(new_size, pOverflow_color);
	new_image.splice(*this, -pMin);
	return new_image;
}

} // namespace wge::graphics
