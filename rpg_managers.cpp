#include "rpg.hpp"
#include "rpg_managers.hpp"

using namespace rpg;

texture_manager::texture_entry*
texture_manager::find_entry(std::string name)
{
	for (auto& i : textures)
	{
		if (i.name == name)
			return &i;
	}
	return nullptr;
}

int
texture_manager::load_settings(std::string path)
{
	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(path.c_str()))
		return 1;

	XMLElement* ele_textures = doc.FirstChildElement("textures");
	if (!ele_textures)
		return 2;

	XMLElement* ele_entry = ele_textures->FirstChildElement();
	while (ele_entry)
	{
		texture_entry nentry;
		nentry.name = ele_entry->Name();
		nentry.path = ele_entry->Attribute("path");

		// the optional atlas path
		const char* opt_atlas = ele_entry->Attribute("atlas");
		if (opt_atlas)
		{
			nentry.atlas = opt_atlas;
			nentry.has_atlas = true;
		}
		else
			nentry.has_atlas = false;

		nentry.is_loaded = false;
		textures.push_back(nentry);
		ele_entry = ele_entry->NextSiblingElement();
	}
	return 0;
}

engine::texture*
texture_manager::get_texture(std::string name)
{
	texture_entry* entry = find_entry(name);
	if (!entry)
	{
		std::cout << "Error: Texture with name '" << name << "' Does not exist.\n";
		return nullptr;
	}
	if (!entry->is_loaded)
	{
		entry->tex.load_texture(entry->path);
		if (entry->has_atlas)
			engine::load_xml_atlas(entry->tex, entry->atlas);
		entry->is_loaded = true;
	}
	return &entry->tex;
}

std::vector<std::string>
texture_manager::construct_list()
{
	std::vector<std::string> retval;
	retval.reserve(textures.size());
	for (auto& i : textures)
		retval.push_back(i.name);
	return std::move(retval);
}

sound_manager::soundbuffer_entry*
sound_manager::find_buffer(std::string name)
{
	for (auto& i : buffers)
		if (i.name == name)
			return &i;
	return nullptr;
}

util::error rpg::sound_manager::load_sounds(std::string path)
{
	return 0;
}

int
sound_manager::spawn_sound(std::string name)
{
	return 0;
}
