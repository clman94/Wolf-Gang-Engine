#include <wge/graphics/texture.hpp>

#include <stb/stb_image.h>

namespace wge::graphics
{

animation::animation(const json & pJson)
{
	load(pJson);
}

void animation::load(const json & pJson)
{
	name = pJson["name"];
	frames = pJson["frames"];
	interval = pJson["interval"];
	frame_rect = pJson["frame-rect"];
}

json animation::save() const
{
	json result;
	result["name"] = name;
	result["frames"] = frames;
	result["interval"] = interval;
	result["frame-rect"] = frame_rect;
	return result;
}

texture::texture(core::asset_config::ptr pConfig) :
	asset(pConfig)
{
	mSmooth = false;
	mPixels = nullptr;
}

texture::~texture()
{
	stbi_image_free(mPixels);
}

void texture::set_implementation(const texture_impl::ptr & pImpl)
{
	mImpl = pImpl;
	// Recreate the texture with the new implementation
	if (mPixels)
	{
		mImpl->create_from_pixels(mPixels, mWidth, mHeight, mChannels);
		mImpl->set_smooth(mSmooth);
	}
}

texture_impl::ptr texture::get_implementation() const
{
	return mImpl;
}

void texture::load(const std::string & pFilepath)
{
	stbi_uc* pixels = stbi_load(pFilepath.c_str(), &mWidth, &mHeight, &mChannels, 4);
	if (!pixels)
	{
		std::cout << "Failed to open image\n";
		return;
	}
	if (mImpl)
		mImpl->create_from_pixels(pixels, mWidth, mHeight, mChannels);
	stbi_image_free(mPixels);
	mPixels = pixels;
	load_metadata();
}

void texture::load(filesystem::stream::ptr pStream, std::size_t pSize)
{
	pSize = (pSize == 0 ? pStream->length() - pStream->tell() : pSize);
	std::vector<char> data;
	data.resize(pSize);
	std::size_t bytes_read = pStream->read(&data[0], pSize);
	stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<unsigned char*>(&data[0]), bytes_read, &mWidth, &mHeight, &mChannels, 4);
	if (!pixels)
	{
		std::cout << "Failed to open image\n";
		return;
	}
	if (mImpl)
		mImpl->create_from_pixels(pixels, mWidth, mHeight, mChannels);
	stbi_image_free(mPixels);
	mPixels = pixels;
	load_metadata();
}

int texture::get_width() const
{
	return mWidth;
}

int texture::get_height() const
{
	return mHeight;
}

void texture::set_smooth(bool pEnabled)
{
	mSmooth = pEnabled;
	mImpl->set_smooth(mSmooth);
}

bool texture::is_smooth() const
{
	return mSmooth;
}

animation::ptr texture::get_animation(const std::string & pName) const
{
	for (const auto& i : mAtlas)
		if (i->name == pName)
			return i;
	return{};
}

texture::atlas_container& texture::get_raw_atlas()
{
	return mAtlas;
}

const texture::atlas_container& texture::get_raw_atlas() const
{
	return mAtlas;
}

void texture::update_metadata() const
{
	json result;
	for (const auto& i : mAtlas)
		result["atlas"].push_back(i->save());
	get_config()->set_metadata(result);
}

void texture::load_metadata()
{
	mAtlas.clear();
	if (!get_config()->get_metadata().is_null())
	{
		const json& atlas = get_config()->get_metadata()["atlas"];
		for (const json& i : atlas)
			mAtlas.push_back(std::make_shared<animation>(i));
	}
	if (!get_animation("default"))
	{
		auto def_animation = std::make_shared<animation>();
		def_animation->name = "Default";
		def_animation->frame_rect = math::rect(0, 0, mWidth, mHeight);
		mAtlas.push_back(def_animation);
	}
}

} // namespace wge::graphics
