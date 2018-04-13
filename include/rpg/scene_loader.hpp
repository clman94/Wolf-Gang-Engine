#ifndef RPG_SCENE_LOADER_HPP
#define RPG_SCENE_LOADER_HPP

#include "../../3rdparty/tinyxml2/tinyxml2.h"

#include <engine/vector.hpp>
#include <engine/rect.hpp>
#include <engine/filesystem.hpp>
#include <engine/resource_pack.hpp>
 
#include <string>
#include <vector>
#include <map>

namespace rpg {

class tilemap_manipulator;
class collision_box_container;

class scene_loader
{
public:
	scene_loader();

	bool load(const engine::generic_path& pDir, const std::string& pName);
	bool load(const engine::generic_path& pDir, const std::string& pName, engine::resource_pack& pPack);
	bool save();

	void clear();

	void set_has_boundary(bool pHas_it);
	bool has_boundary() const;

	void set_boundary(const engine::frect& pRect);
	engine::frect get_boundary() const;

	const std::string& get_name();
	std::string get_script_path() const;

	void set_tilemap_texture(const std::string& pName);
	std::string get_tilemap_texture() const;

	std::string get_scene_path() const;

	void set_collisionboxes(const collision_box_container& pCBC);
	tinyxml2::XMLElement* get_collisionboxes();

	void set_tilemap(const tilemap_manipulator& pTm_m);
	tinyxml2::XMLElement* get_tilemap();

	tinyxml2::XMLDocument& get_document();

private:
	bool load_settings();
	void save_settings();

	// Make xml file well formed
	void fix();
	tinyxml2::XMLDocument      mXml_Document;
	std::string                mScene_name;
	engine::generic_path       mScript_path;
	engine::generic_path       mScene_path;
	std::string mTilemap_texture;
	bool mHas_boundary;
	engine::frect mBoundary;
	tinyxml2::XMLElement* mEle_collisionboxes;
	tinyxml2::XMLElement* mEle_map;
	tinyxml2::XMLElement* mEle_root;
};

}

#endif


