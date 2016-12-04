#include <engine/utility.hpp>
#include <engine/filesystem.hpp>

#include <rpg/rpg_managers.hpp>

using namespace rpg;

int texture_manager::load_from_directory(const std::string& pPath)
{
	if (!engine::fs::exists(pPath))
	{
		util::error("Textures directory does not exist");
		return 1;
	}

	for (auto& i : engine::fs::recursive_directory_iterator(pPath))
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
			if (engine::fs::exists(atlas_path))
				entry.atlas = atlas_path.string();

			// TODO: Possibly preload SMALL textures
		}
	}
	return 0;
}

util::optional_pointer<engine::texture>
texture_manager::get_texture(const std::string& pName)
{
	auto iter = mTextures.find(pName);
	if (iter == mTextures.end())
	{
		util::error("Texture with name '" + pName + "' does not exist.\n");
		return{};
	}

	/// Make sure the texture is loaded
	auto &entry = iter->second;
	if (entry.ensure_loaded())
		return &entry.texture;

	/// Texture failed
	return{};
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
		// load the atlas if it exists
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

int sound_manager::load_from_directory(const std::string& pPath)
{
	if (!engine::fs::exists(pPath))
	{
		util::error("Sound directory does not exist");
		return 1;
	}

	// File with these extensions will be used
	const std::set<std::string> extensions =
	{
		".ogg",
		".flac",
	};

	for (auto& i : engine::fs::recursive_directory_iterator(pPath))
	{
		auto& sound_path = i.path();
		if (extensions.find(sound_path.extension().string()) != extensions.end())
		{
			if (engine::fs::file_size(sound_path) >= 1049000)
			{
				util::warning("It is highly recommended to have sound effects less than 1 MB");
			}

			auto& entry = mBuffers[sound_path.stem().string()];
			if (!entry.load(sound_path.string()))
			{
				util::error("Failed to load sound file '" + sound_path.stem().string() + "'");
			}
		}
	}
	return 0;
}

bool
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
