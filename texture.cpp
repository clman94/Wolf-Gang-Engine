#define ENGINE_INTERNAL

#include "texture.hpp"
#include "tinyxml2\tinyxml2.h"
#include <iostream>

using namespace engine;

int
texture::load_texture(const std::string& path)
{
	if (!_texture.loadFromFile(path))
	{
		return 1;
	}
	return 0;
}

void
texture::add_entry(const std::string& name, const frect rect)
{
	atlas[name] = rect;
}

frect
texture::get_entry(const std::string& name)
{
	return atlas[name];
}

int
texture::load_atlas_xml(const std::string& path)
{
	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(path.c_str()))
		return 1;

	auto atlas_e = doc.FirstChildElement("atlas");
	if (!atlas_e)
		return 2;

	// Iterate though each item and load their values into the atlas
	auto ele_crop = atlas_e->FirstChildElement();
	while (ele_crop)
	{
		frect ncrop;
		ncrop.x = ele_crop->FloatAttribute("x");
		ncrop.y = ele_crop->FloatAttribute("y");
		ncrop.w = ele_crop->FloatAttribute("w");
		ncrop.h = ele_crop->FloatAttribute("h");
		add_entry(ele_crop->Name(), ncrop);

		ele_crop = ele_crop->NextSiblingElement();
	}
	return 0;
}