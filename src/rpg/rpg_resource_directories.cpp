#include <engine/utility.hpp>
#include <engine/filesystem.hpp>
#include <engine/audio.hpp>
#include <engine/utility.hpp>
#include <rpg/rpg_config.hpp>
#include <engine/texture.hpp>
#include <rpg/rpg_resource_directories.hpp>
#include <engine/renderer.hpp>

#include <string>
#include <vector>
#include <list>
#include <set>

#include "../../tinyxml2/xmlshortcuts.hpp"

using namespace rpg;

texture_directory::texture_directory()
{
	mPath = defs::DEFAULT_TEXTURES_PATH.string();
}

bool texture_directory::load(engine::resource_manager& pResource_manager)
{
	if (!engine::fs::exists(mPath))
	{
		util::error("Textures directory does not exist");
		return 1;
	}

	for (auto& i : engine::fs::recursive_directory_iterator(mPath))
	{
		auto& texture_path = i.path();

		if (texture_path.extension() == ".png")
		{
			const std::string texture_name = texture_path.stem().string();
			if (pResource_manager.has_resource(engine::resource_type::texture, texture_name)) // Check if unique
			{
				util::error("Texture '" + texture_name + "' is not unique. Please give it a unique name.");
				continue;
			}
			std::shared_ptr<engine::texture> texture(new engine::texture());
			texture->set_texture_source(texture_path.string());

			// Get atlas path (if it exists)
			auto atlas_path = texture_path.parent_path();
			atlas_path /= texture_name + ".xml";
			if (engine::fs::exists(atlas_path))
				texture->set_atlas_source(atlas_path.string());

			pResource_manager.add_resource(engine::resource_type::texture, texture_name, texture);
		}
	}

	return 0;
}

void texture_directory::set_path(const std::string & pPath)
{
	mPath = pPath;
}

font_directory::font_directory()
{
	mPath = defs::DEFAULT_FONTS_PATH.string();
}

bool font_directory::load(engine::resource_manager& pResource_manager)
{
	if (!engine::fs::exists(mPath))
	{
		util::error("Textures directory does not exist");
		return 1;
	}

	for (auto& i : engine::fs::recursive_directory_iterator(mPath))
	{
		auto& font_path = i.path();

		if (font_path.extension() == ".ttf")
		{
			const std::string font_name = font_path.stem().string();
			if (pResource_manager.has_resource(engine::resource_type::font, font_name)) // Check if unique
			{
				util::error("Texture '" + font_name + "' is not unique. Please give it a unique name.");
				continue;
			}
			std::shared_ptr<engine::font> font(new engine::font());
			font->set_font_source(font_path.string());

			// Get preferences path (if it exists)
			auto preferences_path = font_path.parent_path();
			preferences_path /= font_name + ".xml";
			if (engine::fs::exists(preferences_path))
				font->set_preferences_source(preferences_path.string());

			pResource_manager.add_resource(engine::resource_type::font, font_name, font);
		}
	}
	return 0;
}

void font_directory::set_path(const std::string & pPath)
{
	mPath = pPath;
}

rpg::soundfx_directory::soundfx_directory()
{
	mPath = defs::DEFAULT_SOUND_PATH.string();
}

bool soundfx_directory::load(engine::resource_manager& pResource_manager)
{
	if (!engine::fs::exists(mPath))
	{
		util::error("Sound directory does not exist");
		return false;
	}

	// File with these extensions will be used
	const std::set<std::string> extensions =
	{
		".ogg",
		".flac",
	};

	for (auto& i : engine::fs::recursive_directory_iterator(mPath))
	{
		auto& sound_path = i.path();
		if (extensions.find(sound_path.extension().string()) != extensions.end())
		{
			if (engine::fs::file_size(sound_path) >= 1049000)
			{
				util::warning("It is highly recommended to have sound effects less than 1 MB");
			}

			std::shared_ptr<engine::sound_buffer> buffer(new engine::sound_buffer());
			buffer->set_sound_source(sound_path.string());
			pResource_manager.add_resource(engine::resource_type::sound, sound_path.stem().string(), buffer);
		}
	}
	return true;
}

void soundfx_directory::set_path(const std::string& pPath)
{
	mPath = pPath;
}