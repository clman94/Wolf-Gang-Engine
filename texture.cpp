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
texture::add_entry(const std::string& name, const irect rect)
{
	atlas[name] = rect;
}

irect
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
		irect ncrop;
		ncrop.x = ele_crop->IntAttribute("x");
		ncrop.y = ele_crop->IntAttribute("y");
		ncrop.w = ele_crop->IntAttribute("w");
		ncrop.h = ele_crop->IntAttribute("h");
		add_entry(ele_crop->Name(), ncrop);

		ele_crop = ele_crop->NextSiblingElement();
	}
	return 0;
}