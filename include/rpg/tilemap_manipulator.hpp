#ifndef RPG_TILEMAP_LOADER_HPP
#define RPG_TILEMAP_LOADER_HPP

#include "../../tinyxml2/tinyxml2.h"

#include <engine/renderer.hpp>

#include <rpg/tilemap_display.hpp>

#include <map>

namespace rpg {

class tilemap_manipulator
{
public:
	tilemap_manipulator();

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

	std::string find_tile_name(engine::fvector pPosition, int pLayer);

	void update_display(tilemap_display& tmA);

	void clean();

private:

	class tile
	{
	public:
		typedef engine::vector<unsigned int> fill_t;
		typedef unsigned int rotation_t;

		void set_position(engine::fvector pPosition);
		engine::fvector get_position() const;

		void set_fill(fill_t pFill);
		fill_t get_fill() const;

		void set_rotation(rotation_t pRotation);
		rotation_t get_rotation() const;

		void set_atlas(const std::string& pAtlas);
		const std::string& get_atlas() const;

		void load_xml(tinyxml2::XMLElement* pEle);
		bool is_adjacent_above(tile& a);
		bool is_adjacent_right(tile& a);

		bool operator<(const tile& pTile);

	private:
		engine::fvector mPosition;
		fill_t mFill;
		rotation_t mRotation;
		std::string mAtlas;
	};

	typedef std::map<engine::fvector, tile> layer;
	typedef std::map<int, layer> map;

	map mMap;
	tile* find_tile(engine::fvector pos, int layer);

	engine::fvector mTile_size;

	void condense_layer(layer &pMap);

	int load_layer(tinyxml2::XMLElement *pEle, int pLayer);

	tile* find_tile_at(engine::fvector pPosition, int pLayer);

	void explode_tile(tile* pTile, int pLayer);
};

}

#endif