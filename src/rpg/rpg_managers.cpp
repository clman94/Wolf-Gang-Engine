#include <engine/utility.hpp>

#include <rpg/rpg_managers.hpp>

#ifdef __MINGW32__
#include <experimental/filesystem>
#else
#include <filesystem>
#endif

using namespace rpg;

int texture_manager::load_from_directory(const std::string& pPath)
{
	namespace fs = std::experimental::filesystem;

	if (!fs::exists(pPath))
	{
		util::error("Invalid textures directory does not exist");
		return 1;
	}

	for (auto& i : fs::recursive_directory_iterator(pPath))
	{
		auto& texture_path = i.path();

		if (texture_path.extension() == ".png")
		{
			texture_entry& entry = mTextures[texture_path.stem().string()];
			entry.path = texture_path.string();
			entry.is_loaded = false;

			// Get atlas path (if it exists)
			auto atlas_path = texture_path.parent_path();
			atlas_path /= texture_path.stem().string() + ".xml";
			if (fs::exists(atlas_path))
				entry.atlas = atlas_path.string();
		}
	}
	return 0;
}

engine::texture*
texture_manager::get_texture(const std::string& pName)
{
	auto iter = mTextures.find(pName);
	if (iter == mTextures.end())
	{
		util::error("Texture with name '" + pName + "' does not exist.\n");
		return nullptr;
	}

	auto &entry = iter->second;
	if (entry.ensure_loaded())
		return &entry.texture;

	return nullptr;
}

bool texture_manager::texture_entry::ensure_loaded()
{
	if (!is_loaded)
	{
		if (texture.load_texture(path))
		{
			util::error("Failed to load texture.");
			return false;
		}
		if (!atlas.empty())
			texture.load_atlas_xml(atlas);
		is_loaded = true;
	}
	return true;
}

std::vector<std::string>
texture_manager::construct_list()
{
	std::vector<std::string> retval;
	retval.reserve(mTextures.size());
	for (auto& i : mTextures)
		retval.push_back(i.first);
	return std::move(retval);
}

int
sound_manager::load_sounds(tinyxml2::XMLElement* pEle_root)
{
	auto ele_sound = pEle_root->FirstChildElement();
	while (ele_sound)
	{
		std::string name = ele_sound->Name();

		if (auto att_path = ele_sound->Attribute("path"))
			mBuffers[name].load(att_path);
		else
			util::error("Please provide a path to sound");

		ele_sound = ele_sound->NextSiblingElement();
	}

	return 0;
}

int
sound_manager::spawn_sound(const std::string& name, float pVolume, float pPitch)
{
	auto buffer = mBuffers.find(name);
	if (buffer == mBuffers.end())
		return 1;
	mSounds.spawn(buffer->second, pVolume, pPitch);
	return 0;
}

void sound_manager::stop_all()
{
	mSounds.stop_all();
}
