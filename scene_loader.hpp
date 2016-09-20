#ifndef RPG_SCENE_LOADER_HPP
#define RPG_SCENE_LOADER_HPP

#include <string>
#include "tinyxml2\tinyxml2.h"
#include "vector.hpp"

namespace rpg {

class scene_loader
{
public:
	int load(const std::string& path);
	const engine::fvector& get_boundary();
	const std::string& get_name();
	const std::string& get_script_path();
	const std::string& get_tilemap_texture();
	tinyxml2::XMLElement* get_collisionboxes();
	tinyxml2::XMLElement* get_tilemap();

	tinyxml2::XMLDocument& get_document();

private:
	tinyxml2::XMLDocument mXml_Document;
	std::string mScript_path;
	std::string mScene_name;
	std::string mTilemap_texture;
	engine::fvector mBoundary;
	tinyxml2::XMLElement* mEle_collisionboxes;
	tinyxml2::XMLElement* mEle_map;
};

}

#endif


