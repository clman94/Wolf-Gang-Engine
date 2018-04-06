
#include <engine/utility.hpp>
#include <engine/filesystem.hpp>
#include <engine/audio.hpp>
#include <engine/utility.hpp>
#include <engine/texture.hpp>
#include <engine/renderer.hpp>
#include <engine/logger.hpp>

#include <rpg/rpg_resource_loaders.hpp>
#include <rpg/rpg_config.hpp>

#include <string>
#include <vector>
#include <list>
#include <set>

using namespace rpg;

bool texture_loader::load(engine::resource_manager& pResource_manager, const std::string& mData_filepath)
{
	const engine::fs::path folder_path = mData_filepath + "/" + defs::DEFAULT_TEXTURES_PATH.string();

	if (!engine::fs::exists(folder_path))
	{
		logger::error("Textures directory does not exist");
		return false;
	}

	for (const auto& i : engine::fs::recursive_directory_iterator(folder_path))
	{
		auto& texture_path = i.path();

		if (texture_path.extension() == ".png")
		{
			const std::string texture_name = texture_path.stem().string();

			// Check if unique
			if (pResource_manager.has_resource("texture", texture_name))
			{
				logger::warning("Texture '" + texture_name + "' is not unique. Please give it a unique name.");
				continue;
			}

			std::shared_ptr<engine::texture> texture(std::make_shared<engine::texture>());
			texture->set_name(texture_name);
			texture->set_texture_source(texture_path.string());

			// Parse atlas (if it exists)
			auto atlas_path = texture_path.parent_path();
			atlas_path /= texture_name + ".xml";
			if (engine::fs::exists(atlas_path))
			{
				texture->set_atlas_source(atlas_path.string());
			}
			else
			{
				logger::warning("Atlas of texture '" + texture_name + "' does not exist");
				continue; // Texture requires atlas
			}

			pResource_manager.add_resource(texture);
		}
	}

	return true;
}

bool texture_loader::load_pack(engine::resource_manager & pResource_manager, engine::resource_pack & pPack)
{
	auto file_list = pPack.recursive_directory(defs::DEFAULT_TEXTURES_PATH.string());
	for (auto &i : file_list)
	{
		if (i.extension() == ".png")
		{
			const std::string texture_name = i.stem();

			std::shared_ptr<engine::texture> texture(std::make_shared<engine::texture>());
			texture->set_name(texture_name);
			texture->set_texture_source(i.string());

			// Parse atlas xml (if it exists)
			auto atlas_path = i.parent() / (texture_name + ".xml");
			texture->set_atlas_source(atlas_path.string());

			pResource_manager.add_resource(texture);
		}
	}
	return true;
}

bool font_loader::load(engine::resource_manager& pResource_manager, const std::string& mData_filepath)
{
	const engine::fs::path folder_path = mData_filepath + "/" + defs::DEFAULT_FONTS_PATH.string();

	if (!engine::fs::exists(folder_path))
	{
		logger::error("Textures directory does not exist");
		return false;
	}

	for (const auto& i : engine::fs::recursive_directory_iterator(folder_path))
	{
		auto& font_path = i.path();

		if (font_path.extension() == ".ttf")
		{
			const std::string font_name = font_path.stem().string();

			// Check if unique
			if (pResource_manager.has_resource("font", font_name))
			{
				logger::warning("Font '" + font_name + "' is not unique. Please give it a unique name.");
				continue;
			}

			std::shared_ptr<engine::font> font(std::make_shared<engine::font>());
			font->set_name(font_name);
			font->set_font_source(font_path.string());

			// Parse preferences xml file (if it exists)
			auto preferences_path = font_path.parent_path();
			preferences_path /= font_name + ".xml";
			if (engine::fs::exists(preferences_path))
				font->set_preferences_source(preferences_path.string());

			pResource_manager.add_resource(font);
		}
	}
	return true;
}

bool font_loader::load_pack(engine::resource_manager & pResource_manager, engine::resource_pack & pPack)
{
	auto file_list = pPack.recursive_directory(defs::DEFAULT_FONTS_PATH.string());
	for (auto& i : file_list)
	{
		if (i.extension() == ".ttf")
		{
			const std::string font_name = i.stem();

			std::shared_ptr<engine::font> font(std::make_shared<engine::font>());
			font->set_name(font_name);
			font->set_font_source(i.string());

			auto preferences_path = i.parent() / (font_name + ".xml");
			font->set_preferences_source(preferences_path.string());

			pResource_manager.add_resource(font);
		}
	}
	return true;
}

// Files with these extensions will be used
static const std::set<std::string> supported_sound_extensions =
{
	".ogg",
	".flac",
};


bool audio_loader::load(engine::resource_manager & pResource_manager, const std::string& mData_filepath)
{
	const engine::fs::path folder_path = engine::fs::path(mData_filepath) / defs::DEFAULT_AUDIO_PATH;

	if (!engine::fs::exists(folder_path))
	{
		logger::error("Audio directory does not exist");
		return false;
	}

	for (auto& i : engine::fs::recursive_directory_iterator(folder_path))
	{
		auto& sound_path = i.path();
		if (supported_sound_extensions.find(sound_path.extension().string()) != supported_sound_extensions.end())
		{
			std::shared_ptr<engine::sound_file> buffer(std::make_shared<engine::sound_file>());
			buffer->set_name(sound_path.stem().string());
			buffer->set_filepath(sound_path.string());
			pResource_manager.add_resource(buffer);
		}
	}
	return true;
}

bool audio_loader::load_pack(engine::resource_manager & pResource_manager, engine::resource_pack & pPack)
{
	auto file_list = pPack.recursive_directory(defs::DEFAULT_AUDIO_PATH);
	for (auto& i : file_list)
	{
		if (supported_sound_extensions.find(i.extension()) != supported_sound_extensions.end())
		{
			std::shared_ptr<engine::sound_file> buffer(std::make_shared<engine::sound_file>());
			buffer->set_name(i.stem());
			buffer->set_filepath(i.string());
			pResource_manager.add_resource(buffer);
		}
	}
	return true;
}

bool script_loader::load(engine::resource_manager & pResource_manager, const std::string & mData_filepath)
{
	return false;
}

bool script_loader::load_pack(engine::resource_manager & pResource_manager, engine::resource_pack & pPack)
{
	return false;
}
