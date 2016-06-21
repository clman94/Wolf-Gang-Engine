#include "rpg.hpp"

using namespace rpg;

texture_manager::texture_entry*
texture_manager::find_entry(std::string name)
{
	for (auto& i : texturebank)
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

	XMLElement* texbank_e = doc.FirstChildElement("textures");
	if (!texbank_e)
		return 2;

	XMLElement* entry = texbank_e->FirstChildElement();
	while (entry)
	{
		texture_entry nentry;
		nentry.name = entry->Name();
		nentry.path = entry->Attribute("path");

		// the optional atlas path
		const char* opt_atlas = entry->Attribute("atlas");
		if (opt_atlas)
		{
			nentry.atlas = opt_atlas;
			nentry.has_atlas = true;
		}
		else
			nentry.has_atlas = false;

		nentry.is_loaded = false;
		texturebank.push_back(nentry);
		entry = entry->NextSiblingElement();
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
			entry->tex.load_atlas(entry->atlas);
		entry->is_loaded = true;
	}
	return &entry->tex;
}
