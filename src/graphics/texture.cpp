#include <wge/graphics/texture.hpp>

#include <stb/stb_image.h>
#include <fstream>
#include <sstream>

namespace wge::graphics
{

void texture::set_implementation(const texture_impl::ptr& pImpl) noexcept
{
	mImpl = pImpl;
	update_impl_image();
}

texture_impl::ptr texture::get_implementation() const noexcept
{
	return mImpl;
}

void texture::set_image(image&& pImage)
{
	mImage = std::move(pImage);
	update_impl_image();
}

void texture::set_image(const image& pImage)
{
	mImage = pImage;
	update_impl_image();
}

int texture::get_width() const noexcept
{
	return mImage.get_width();
}

int texture::get_height() const noexcept
{
	return mImage.get_height();
}

math::ivec2 texture::get_size() const noexcept
{
	return mImage.get_size();
}

void texture::set_smooth(bool pEnabled) noexcept
{
	mSmooth = pEnabled;
	mImpl->set_smooth(mSmooth);
}

bool texture::is_smooth() const noexcept
{
	return mSmooth;
}

void texture::update_impl_image()
{
	if (!mImage.empty() && mImpl)
	{
		mImpl->create_from_image(mImage);
		mImpl->set_smooth(mSmooth);
	}
}

} // namespace wge::graphics
