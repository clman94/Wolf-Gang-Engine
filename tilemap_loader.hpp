#ifndef RPG_TILEMAP_LOADER_HPP
#define RPG_TILEMAP_LOADER_HPP

#include "renderer.hpp"
#include "tilemap_display.hpp"
#include "tinyxml2\tinyxml2.h"
namespace rpg {

class tilemap_loader
{
public:
	tilemap_loader();

	void condense_tiles();

	util::error load_tilemap_xml(tinyxml2::XMLElement *root);
	util::error load_tilemap_xml(std::string pPath);

	void break_tile(engine::fvector pPosition, size_t pLayer);

	void generate(tinyxml2::XMLDocument& doc, tinyxml2::XMLNode* root);
	void generate(const std::string& pPath);

	int  set_tile(engine::fvector pPosition, engine::fvector pFill, size_t pLayer, const std::string& pAtlas, int pRotation);
	int  set_tile(engine::fvector pPosition, size_t pLayer, const std::string& pAtlas, int pRotation);
	void remove_tile(engine::fvector pPosition, size_t pLayer);

	void update_display(tilemap_display& tmA);

	void clean();

private:
	struct tile
	{
		engine::fvector pos, fill;
		int rotation;
		std::string atlas;
		void load_xml(tinyxml2::XMLElement *e, size_t layer);
		bool is_adjacent_above(tile& a);
		bool is_adjacent_right(tile& a);
	};
	std::map<size_t, std::vector<tile>> mTiles;
	tile* find_tile(engine::fvector pos, size_t layer);

	engine::fvector mTile_size;

	void condense_layer(std::vector<tile> &pMap);

	util::error load_layer(tinyxml2::XMLElement *pEle, size_t pLayer);

	tile* find_tile_at(engine::fvector pPosition, size_t pLayer);
};

}

#endif