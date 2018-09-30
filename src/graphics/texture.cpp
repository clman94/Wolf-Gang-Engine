#include <wge/graphics/texture.hpp>

#include <stb/stb_image.h>

using namespace wge;
using namespace wge::graphics;

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
	mGL_texture = 0;
	mSmooth = false;
	mPixels = nullptr;
}

texture::~texture()
{
	stbi_image_free(mPixels);
}

void texture::load(const std::string & pFilepath)
{
	stbi_uc* pixels = stbi_load(pFilepath.c_str(), &mWidth, &mHeight, &mChannels, 4);
	if (!pixels)
	{
		std::cout << "Failed to open image\n";
		return;
	}
	create_from_pixels(pixels);
	stbi_image_free(mPixels);
	mPixels = pixels;
}

void texture::load(filesystem::input_stream::ptr pStream, std::size_t pSize)
{
	pSize = (pSize == 0 ? pStream->length() - pStream->tell() : pSize);
	std::vector<unsigned char> data;
	data.resize(pSize);
	std::size_t bytes_read = pStream->read(&data[0], pSize);
	stbi_uc* pixels = stbi_load_from_memory(&data[0], bytes_read, &mWidth, &mHeight, &mChannels, 4);
	if (!pixels)
	{
		std::cout << "Failed to open image\n";
		return;
	}
	create_from_pixels(pixels);
	stbi_image_free(mPixels);
	mPixels = pixels;
}

GLuint texture::get_gl_texture() const
{
	return mGL_texture;
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
	update_filtering();
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
}

texture::atlas_container & texture::get_raw_atlas()
{
	return mAtlas;
}

const texture::atlas_container & texture::get_raw_atlas() const
{
	return mAtlas;
}

void texture::create_from_pixels(unsigned char * pBuffer)
{
	if (!mGL_texture)
	{
		// Create the texture object
		glGenTextures(1, &mGL_texture);
	}

	// Give the image to OpenGL
	glBindTexture(GL_TEXTURE_2D, mGL_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pBuffer);
	glBindTexture(GL_TEXTURE_2D, 0);

	update_filtering();
}

void texture::update_filtering()
{
	glBindTexture(GL_TEXTURE_2D, mGL_texture);

	GLint filtering = mSmooth ? GL_LINEAR : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);

	glBindTexture(GL_TEXTURE_2D, 0);
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
	const json& atlas = get_config()->get_metadata()["atlas"];
	for (const json& i : atlas)
		mAtlas.push_back(std::make_shared<animation>(i));
}
