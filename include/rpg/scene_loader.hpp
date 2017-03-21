#ifndef RPG_SCENE_LOADER_HPP
#define RPG_SCENE_LOADER_HPP

#include "../../tinyxml2/tinyxml2.h"
#include "../../src/xmlshortcuts.hpp"

#include <engine/vector.hpp>
#include <engine/rect.hpp>
#include <engine/utility.hpp>
#include <engine/filesystem.hpp>
#include <rpg/collision_box.hpp>
 
#include <string>
#include <vector>
#include <map>

namespace rpg {

std::vector<std::string> get_scene_list();

typedef std::map<std::string, std::vector<size_t>> wall_groups_t;

class scene_loader
{
public:
	scene_loader();

	bool load(const std::string& pName);
	bool save();

	void clean();
	bool has_boundary() const;
	const engine::frect& get_boundary() const;
	const std::string& get_name();
	std::string get_script_path() const;
	std::string get_tilemap_texture() const;
	std::string get_scene_path() const;

	util::optional_pointer<tinyxml2::XMLElement> get_collisionboxes();
	util::optional_pointer<tinyxml2::XMLElement> get_tilemap();

	tinyxml2::XMLDocument& get_document();

private:

	// Make xml file well formed
	void fix();
	tinyxml2::XMLDocument      mXml_Document;
	std::string                mScene_name;
	engine::fs::path           mScript_path;
	engine::fs::path           mTilemap_texture;
	engine::fs::path           mScene_path;
	engine::frect              mBoundary;
	bool                       mHas_boundary;
	util::optional_pointer<tinyxml2::XMLElement> mEle_collisionboxes;
	util::optional_pointer<tinyxml2::XMLElement> mEle_map;
};

}

#endif


