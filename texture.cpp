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

void
texture::add_entry(const std::string name, const engine::irect rect)
{
	texture_crop ncrop;
	ncrop.set_rect(rect);
	ncrop.name = name;
	atlas.push_back(ncrop);
}

irect
texture::get_entry(const std::string name)
{
	for (auto &i : atlas)
	{
		if (i.name == name)
			return i;
	}
	return irect();
}

int
engine::load_xml_atlas(texture& tex, const std::string path)
{
	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(path.c_str()))
		return 1;

	auto atlas_e = doc.FirstChildElement("atlas");
	if (!atlas_e)
		return 2;

	// Iterate though each item and load their values into the atlas
	auto crop_param = atlas_e->FirstChildElement();
	while (crop_param)
	{
		irect ncrop;
		ncrop.x = crop_param->IntAttribute("x");
		ncrop.y = crop_param->IntAttribute("y");
		ncrop.w = crop_param->IntAttribute("w");
		ncrop.h = crop_param->IntAttribute("h");
		tex.add_entry(crop_param->Name(), ncrop);

		crop_param = crop_param->NextSiblingElement();
	}
	return 0;
}