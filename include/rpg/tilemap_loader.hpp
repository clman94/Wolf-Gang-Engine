#ifndef RPG_TILEMAP_LOADER_HPP
#define RPG_TILEMAP_LOADER_HPP

#include "../../tinyxml2/tinyxml2.h"

#include <engine/renderer.hpp>

#include <rpg/tilemap_display.hpp>

#include <map>

namespace rpg {

class tilemap_loader
{
public:
	tilemap_loader();

	void condense_tiles();

	int load_tilemap_xml(tinyxml2::XMLElement *root);
	int load_tilemap_xml(std::string pPath);

	void explode_tile(engine::fvector pPosition, int pLayer);
	void explode_all();

	void generate(tinyxml2::XMLDocument& doc, tinyxml2::XMLNode* root);
	void generate(const std::string& pPath);

	int  set_tile(engine::fvector pPosition, engine::fvector pFill, int pLayer, const std::string& pAtlas, int pRotation);
	int  set_tile(engine::fvector pPosition, int pLayer, const std::string& pAtlas, int pRotation);
	void remove_tile(engine::fvector pPosition, int pLayer);

	void update_display(tilemap_display& tmA);

	void clean();

private:


	struct tile
	{
		engine::fvector position, fill;
		int rotation;
		std::string atlas;
		void load_xml(tinyxml2::XMLElement *e, size_t layer);
		bool is_adjacent_above(tile& a);
		bool is_adjacent_right(tile& a);
	};

	typedef std::map<engine::fvector, tile> layer;
	typedef std::map<int, layer> map;

	map mMap;
	tile* find_tile(engine::fvector pos, size_t layer);

	engine::fvector mTile_size;

	void condense_layer(layer &pMap);

	int load_layer(tinyxml2::XMLElement *pEle, int pLayer);

	tile* find_tile_at(engine::fvector pPosition, int pLayer);

	void explode_tile(tile* pTile, int pLayer);
};

}

#endif