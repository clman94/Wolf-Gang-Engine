#ifndef RPG_TILEMAP_MANIPULATOR_HPP
#define RPG_TILEMAP_MANIPULATOR_HPP

#include "../../3rdparty/tinyxml2/tinyxml2.h"

#include <engine/renderer.hpp>
#include <engine/texture.hpp>

#include <map>
#include <memory>
#include <string>

namespace rpg {

// This saves us some memory by sharing the atlas names of the tiles.
// It also provides a correction feature to fix old tiles when atlas
// names change in the texture.
class tile_atlas_pool
{
public:
	typedef std::shared_ptr<const std::string> handle;

	// Get a shared pointer version of a string from the pool.
	handle get(const std::string& pAtlas);

	// Replace an entry. All tile atlases are shared so everything changes smoothly.
	// Returns true if successful.
	bool replace(const std::string& pOriginal, const std::string& pNew);

	// Compile a list of entries that don't exist in the texture.
	// This info can be used to replace tiles with atlas names that have been changed.
	std::vector<std::string> get_invalid_entries(std::shared_ptr<engine::texture> pTexture) const;

	bool has_invalid_entries(std::shared_ptr<engine::texture> pTexture) const;

private:
	std::vector<std::shared_ptr<std::string>> mStrings;
};

class tile
{
public:
	tile();
	tile(tile&& pMove);
	tile(const tile& pTile);

	tile& operator=(const tile& pTile);
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

	void set_atlas(const std::string& pAtlas, tile_atlas_pool& pPool);
	void set_atlas(tile_atlas_pool::handle pHandle);
	const std::string& get_atlas() const;

	// Load tile settings from xml. The pool is needed for the atlas names.
	void load_xml(tinyxml2::XMLElement* pEle, tile_atlas_pool& pPool);

	bool is_adjacent_above(tile& a);
	bool is_adjacent_left(tile& a);

	bool is_condensed() const;

	bool operator<(const tile& pTile);

private:
	engine::fvector mPosition;
	fill_t mFill;
	rotation_t mRotation;
	tile_atlas_pool::handle mAtlas_handle;
};

class tilemap_layer
{
public:
	void set_name(const std::string& pName);
	const std::string& get_name() const;

	tile* new_tile(const std::string& pAtlas);

	// Set a tile. Adds automatically and replaces any at the same position.
	tile* set_tile(engine::fvector pPosition, engine::uvector pFill, const std::string& pAtlas, int pRotation);
	
	// Set a tile. Adds automatically and replaces any at the same position.
	tile* set_tile(engine::fvector pPosition, const std::string& pAtlas, int pRotation);

	tile* set_tile(const tile& pTile);

	// Find tile at position
	tile* find_tile(engine::fvector pPosition);

	size_t get_tile_count() const;
	tile* get_tile(size_t pIndex);
	const tile* get_tile(size_t pIndex) const;

	bool remove_tile(engine::fvector pPosition);

	// Take all adjacent tiles and represent them with only on tile.
	// Useful to save space when saving the tilemap.
	// Returns a ratio New:Original size; 1 = No change
	float condense();

	void explode_tile(tile* pTile);

	// Explode all condensed tiles into many tiles.
	// It is recommended calling this before modifying tiles.
	void explode();

	// Load this layer from xml settings.
	bool load_xml(tinyxml2::XMLElement *pRoot);

	void generate_xml(tinyxml2::XMLElement *pRoot, tinyxml2::XMLDocument& doc) const;

	tile_atlas_pool& get_pool();

	engine::fvector get_center_point() const;

	void sort();

private:
	tile_atlas_pool mAtlas_pool;
	std::vector<tile> mTiles;
	std::string mName;
};

class tilemap_manipulator
{
public:
	void condense_map();

	int load_tilemap_xml(tinyxml2::XMLElement *root);
	int load_tilemap_xml(std::string pPath);

	void explode_all();

	void save_xml(tinyxml2::XMLDocument& doc, tinyxml2::XMLNode* root) const;
	void save_xml(const std::string& pPath) const;

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

private:
	std::vector<tilemap_layer> mMap;
};

}

#endif