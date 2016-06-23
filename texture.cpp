#define ENGINE_INTERNAL

#include "texture.hpp"
#include "tinyxml2\tinyxml2.h"
#include <iostream>

using namespace engine;

int
texture::load_texture(const std::string path)
{
	if (!_texture.loadFromFile(path))
	{
		return 1;
	}
	return 0;
}

int
texture::load_atlas(const std::string path)
{
	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(path.c_str()))
		return 1;

	XMLElement* atlas_e = doc.FirstChildElement("atlas");
	if (!atlas_e)
		return 2;

	// Iterate though each item and load their values into the atlas
	XMLElement* crop_param = atlas_e->FirstChildElement();
	while (crop_param)
	{
		texture_crop ncrop;
		ncrop.name = crop_param->Name();
		ncrop.id   = crop_param->IntAttribute("id");
		ncrop.x    = crop_param->IntAttribute("x");
		ncrop.y    = crop_param->IntAttribute("y");
		ncrop.w    = crop_param->IntAttribute("w");
		ncrop.h    = crop_param->IntAttribute("h");
		atlas.push_back(ncrop);

		crop_param = crop_param->NextSiblingElement();
	}
	return 0;
}

int
texture::find_atlas(std::string name, texture_crop& crop)
{
	for (auto i : atlas)
	{
		if (i.name == name)
		{
			crop = i;
			return 0;
		}
	}
	return 1;
}

int
texture::find_atlas(int id, texture_crop& crop)
{
	for (auto i : atlas)
	{
		if (i.id == id)
		{
			crop = i;
			return 0;
		}
	}
	return 1;
}