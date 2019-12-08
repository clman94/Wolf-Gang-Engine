#include <wge/graphics/texture.hpp>

#include <stb/stb_image.h>
#include <fstream>
#include <sstream>

namespace wge::graphics
{

animation::animation(const json& pJson)
{
	deserialize(pJson);
}

void animation::deserialize(const json& pJson)
{
	name = pJson["name"];
	frames = pJson["frames"];
	interval = pJson["interval"];
	frame_rect = pJson["frame-rect"];
	id = pJson["id"];
}

json animation::serialize() const
{
	json result;
	result["name"] = name;
	result["frames"] = frames;
	result["interval"] = interval;
	result["frame-rect"] = frame_rect;
	result["id"] = id;
	return result;
}


void texture::set_implementation(const texture_impl::ptr& pImpl) noexcept
{
	mImpl = pImpl;
	if (!mImage.empty() && mImpl)
	{
		// Recreate the texture with the new implementation
		mImpl->create_from_image(mImage);
		mImpl->set_smooth(mSmooth);
	}
}

texture_impl::ptr texture::get_implementation() const noexcept
{
	return mImpl;
}

void texture::load(const filesystem::path& pDirectory, const std::string& pName)
{
	mPath = pDirectory / (pName + ".png");
	mImage.load_file(mPath.string());
	
	if (mImpl)
		mImpl->create_from_image(mImage);
}

int texture::get_width() const noexcept
{
	return mImage.get_width();
}

int texture::get_height() const noexcept
{
	return mImage.get_height();
}

math::vec2 texture::get_size() const noexcept
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

animation* texture::get_animation(const std::string& pName) noexcept
{
	for (auto& i : mAtlas)
		if (i.name == pName)
			return &i;
	return nullptr;
}

animation* texture::get_animation(const util::uuid& pId) noexcept
{
	for (auto& i : mAtlas)
		if (i.id == pId)
			return &i;
	return nullptr;
}

texture::atlas_container& texture::get_raw_atlas() noexcept
{
	return mAtlas;
}

const texture::atlas_container& texture::get_raw_atlas() const noexcept
{
	return mAtlas;
}

void texture::update_source_path(const filesystem::path& pDirectory, const std::string& pName)
{
	auto old_path = pDirectory / mPath.filename();
	auto new_path = pDirectory / (pName + ".png");

	// Rename the texture file for name consistancy
	system_fs::rename(old_path, new_path);
	mPath = std::move(new_path);
}

json texture::serialize_data() const
{
	json result;
	for (const auto& i : mAtlas)
		result["atlas"].push_back(i.serialize());
	return result;
}

void texture::deserialize_data(const json& pJson)
{
	mAtlas.clear();
	if (!pJson.is_null())
	{
		const json& atlas = pJson["atlas"];
		for (const json& i : atlas)
			mAtlas.emplace_back(i);
	}
	if (!get_animation("Default"))
	{
		animation& def_animation = mAtlas.emplace_back();
		def_animation.name = "Default";
		def_animation.id = util::generate_uuid();
		def_animation.frame_rect = math::rect(math::vec2(0, 0), mImage.get_size());
	}
}

} // namespace wge::graphics
