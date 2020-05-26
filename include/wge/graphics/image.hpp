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
	static constexpr std::size_t channel_count = 4;

public:
	image() = default;
	image(const math::ivec2& pSize, const color8& pColor) :
		mSize(pSize)
	{
		mData.resize(static_cast<std::size_t>(mSize.x) * static_cast<std::size_t>(mSize.y) * channel_count);
		fill(pColor);
	}

	bool save_png(const std::string& pPath) const;
	bool load_file(const std::string& pPath);

	// Get vec2 of the size of this image in pixels.
	math::ivec2 get_size() const noexcept;

	// Get width of this image in pixels.
	int get_width() const noexcept;

	// Get height of this image in pixels.
	int get_height() const noexcept;

	void splice(const image& pSrc, const math::ivec2& pDest) noexcept
	{
		splice(pSrc, pDest, { 0, 0 }, pSrc.get_size() - math::ivec2{ 1, 1 });
	}

	void splice(const image& pSrc,
		const math::ivec2& pDest,
		const math::ivec2& pSrc_min,
		const math::ivec2& pSrc_max) noexcept;

	template <typename Tcallable>
	void transform(Tcallable&& pCallable)
	{
		for (int y = 0; y < mSize.y; y++)
		{
			for (int x = 0; x < mSize.x; x++)
			{
				const color8 new_pixel = pCallable(get_pixel({ x, y }), math::ivec2{ x, y });
				set_pixel({ x, y }, new_pixel);
			}
		}
	}

	image crop(const math::ivec2& pMin, const math::ivec2& pMax, const color8& pOverflow_color = { 0, 0, 0, 0 }) const;

	bool empty() const noexcept
	{
		return mData.empty();
	}

	void fill(const color8& pColor) noexcept
	{
		assert(mData.size() % 4 == 0);
		for (std::size_t i = 0; i < mData.size(); i += 4)
		{
			mData[i    ] = pColor.r;
			mData[i + 1] = pColor.g;
			mData[i + 2] = pColor.b;
			mData[i + 3] = pColor.a;
		}
	}

	auto get_raw_pixel(const math::ivec2& pPos) noexcept
	{
		assert(!mData.empty());
		assert(pPos.x >= 0);
		assert(pPos.x < mSize.x);
		assert(pPos.y >= 0);
		assert(pPos.y < mSize.y);
		return util::span{ &mData[pixel_index(pPos)], channel_count };
	}

	auto get_raw_pixel(const math::ivec2& pPos) const noexcept
	{
		assert(!mData.empty());
		assert(pPos.x >= 0);
		assert(pPos.x < mSize.x);
		assert(pPos.y >= 0);
		assert(pPos.y < mSize.y);
		return util::span{ &mData[pixel_index(pPos)], channel_count };
	}

	void set_pixel(const math::ivec2& pPos, const color8& pColor)
	{
		auto pixel = get_raw_pixel(pPos);
		pixel[0] = pColor.r;
		pixel[1] = pColor.g;
		pixel[2] = pColor.b;
		pixel[3] = pColor.a;
	}

	color8 get_pixel(const math::ivec2& pPos) const noexcept
	{
		return get_raw_pixel(pPos);
	}

	auto get_raw() const noexcept
	{
		return util::span{ mData };
	}

private:
	std::size_t pixel_index(const math::ivec2& pPos) const noexcept
	{
		return (static_cast<std::size_t>(pPos.x) +
			static_cast<std::size_t>(pPos.y) *
			static_cast<std::size_t>(mSize.x)) *
			channel_count;
	}

private:
	math::ivec2 mSize;
	std::vector<color8::type> mData;

};

} // namespace wge::graphics
