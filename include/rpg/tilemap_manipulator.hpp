#ifndef RPG_TILEMAP_MANIPULATOR_HPP
#define RPG_TILEMAP_MANIPULATOR_HPP

#include "../../3rdparty/tinyxml2/tinyxml2.h"

#include <engine/renderer.hpp>
#include <engine/texture.hpp>

#include <map>
#include <memory>
#include <string>

namespace rpg {

class tile
{
public:
	tile();
	tile(tile&& pMove);
	tile(const tile& pTile);

	tile& operator=(const tile& pRight);
	bool operator==(const tile& pRight);
	bool operator!=(const tile& pRight);

	typedef engine::uvector fill_t;
	typedef unsigned int rotation_t;

	void set_position(engine::fvector pPosition);
	engine::fvector get_position() const;

	void set_fill(fill_t pFill);
	fill_t get_fill() const;

	void set_rotation(rotation_t pRotation);
	rotation_t get_rotation() const;

	void set_atlas(engine::subtexture::ptr pAtlas);
	engine::subtexture::ptr get_atlas() const;

	bool load_xml(tinyxml2::XMLElement* pEle, std::shared_ptr<engine::texture>& pTexture);
	void save_xml(tinyxml2::XMLElement* pEle) const;

	bool is_adjacent_above(tile& a);
	bool is_adjacent_left(tile& a);

	bool is_condensed() const;

	bool operator<(const tile& pTile);

private:
	engine::fvector mPosition;
	fill_t mFill;
	rotation_t mRotation;
	engine::subtexture::ptr mAtlas;
};

class tilemap_layer
{
public:
	void set_name(const std::string& pName);
	const std::string& get_name() const;

	tile* new_tile(const std::string& pAtlas);

	tile* set_tile(engine::fvector pPosition, engine::uvector pFill, const std::string& pAtlas, int pRotation); // Replaces any tiles at the same position; otherwise it adds it
	tile* set_tile(engine::fvector pPosition, const std::string& pAtlas, int pRotation);                        // "
	tile* set_tile(const tile& pTile);

	// Find tile at the EXACT position
	tile* find_tile(engine::fvector pPosition);

	//std::vector<tile*> get_intersecting_tiles(engine::fvector pPosition); // Set the texture before calling
	//std::vector<tile*> get_intersecting_tiles(engine::frect pRect);       // "

	size_t get_tile_count() const;
	tile* get_tile(size_t pIndex);
	const tile* get_tile(size_t pIndex) const;

	bool remove_tile(engine::fvector pPosition);

	// Take all adjacent tiles and represent them with only on tile.
	// Useful to save space when saving the tilemap.
	// Returns a ratio New/Original size; 1 = No change
	float condense();

	void explode_tile(tile* pTile);

	// Explode all condensed tiles into many tiles.
	// It is recommended calling this before modifying tiles.
	void explode();

	bool load_xml(tinyxml2::XMLElement *pRoot);
	void save_xml(tinyxml2::XMLElement *pRoot, tinyxml2::XMLDocument& doc) const;

	engine::fvector get_center_point() const;

	void sort();

	void set_texture(std::shared_ptr<engine::texture> pTexture); // Call this before doing ANYTHING

private:
	std::vector<tile> mTiles;
	std::string mName;
	std::shared_ptr<engine::texture> mTexture;

	struct invalid_tile_info
	{
		std::string requested_atlas; // Name of atlas entry that the tile requests but couldn't find
		tile incomplete_tile;
	};
	std::vector<invalid_tile_info> invalid_tiles;
};

class tilemap_manipulator
{
public:
	int load_xml(tinyxml2::XMLElement *root);
	int load_xml(std::string pPath);
	void save_xml(tinyxml2::XMLDocument& doc, tinyxml2::XMLNode* root) const;
	void save_xml(const std::string& pPath) const;

	void condense_all();
	void explode_all();

	bool move_layer(size_t pFrom, size_t pTo);

	void clear();

	engine::fvector get_center_point() const;

	// Add a new layer to the bottom.
	// Returns index of the new layer.
	size_t new_layer();

	// Insert a layer at an index.
	// Returns the the index of the new layer.
	size_t insert_layer(size_t pIndex);

	void remove_layer(size_t pIndex);

	size_t get_layer_count() const;
	tilemap_layer& get_layer(size_t pIndex);
	const tilemap_layer& get_layer(size_t pIndex) const;

	void set_texture(std::shared_ptr<engine::texture> pTexture); // Call this before doing ANYTHING
	std::shared_ptr<engine::texture> get_texture() const;

private:
	std::vector<tilemap_layer> mMap;
	std::shared_ptr<engine::texture> mTexture;
};

}

#endif