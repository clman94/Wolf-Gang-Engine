#include <rpg/tilemap_manipulator.hpp>
#include <rpg/rpg_config.hpp>
#include <engine/logger.hpp>
#include <rpg/scene_loader.hpp>

using namespace rpg;

void tile::load_xml(tinyxml2::XMLElement * pEle, tile_atlas_pool& pPool)
{
	assert(pEle != nullptr);

	set_atlas(util::safe_string(pEle->Name()), pPool);

	mPosition.x = pEle->FloatAttribute("x");
	mPosition.y = pEle->FloatAttribute("y");

	mFill.x = pEle->UnsignedAttribute("w");
	mFill.y = pEle->UnsignedAttribute("h");

	// Default fill to 1 for 1x1 tile
	mFill.x = (mFill.x == 0 ? 1 : mFill.x);
	mFill.y = (mFill.y == 0 ? 1 : mFill.y);

	mRotation = pEle->UnsignedAttribute("r") % 4;
}

bool tile::is_adjacent_above(tile & a)
{
	return (
		mAtlas_handle == a.mAtlas_handle
		&& mPosition.x == a.mPosition.x
		&& mPosition.y + static_cast<float>(mFill.y) == a.mPosition.y
		&& mFill.x == a.mFill.x
		&& mRotation == a.mRotation
		);
}

bool tile::is_adjacent_left(tile & a)
{
	return (
		mAtlas_handle == a.mAtlas_handle
		&& mPosition.y == a.mPosition.y
		&& mPosition.x + static_cast<float>(mFill.x) == a.mPosition.x
		&& mFill.y == a.mFill.y
		&& mRotation == a.mRotation
		);
}

bool tile::is_condensed() const
{
	return mFill.x > 1 || mFill.y > 1;
}

bool tile::operator<(const tile & pTile)
{
	return mPosition < pTile.mPosition;
}

void tilemap_manipulator::condense_map()
{
	if (mMap.empty())
		return;

	float sum = 0;
	for (auto &i : mMap)
	{
		float r = i.condense();
		logger::info("Condensed layer '" + i.get_name() + "' to " + std::to_string(r*100) + "%");
		sum += r;
	}
	logger::info("Tilemap condensed to " + std::to_string(sum / static_cast<float>(mMap.size()) * 100) + "%");
}

int tilemap_manipulator::load_tilemap_xml(tinyxml2::XMLElement *pRoot)
{
	clear();

	if (auto att_path = pRoot->Attribute("path"))
	{
		load_tilemap_xml(util::safe_string(att_path));
	}

	auto ele_layer = pRoot->FirstChildElement("layer");
	while (ele_layer)
	{
		int att_id = ele_layer->IntAttribute("id");
		tilemap_layer& nLayer = mMap[new_layer()];
		nLayer.load_xml(ele_layer);
		ele_layer = ele_layer->NextSiblingElement("layer");
	}
	return 0;
}

int tilemap_manipulator::load_tilemap_xml(std::string pPath)
{
	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
	{
		logger::error("Error loading tilemap file");
		return 1;
	}
	auto root = doc.RootElement();
	load_tilemap_xml(root);
	return 0;
}

void tilemap_manipulator::explode_all()
{
	for (auto &i : mMap)
		i.explode();
}

void tilemap_manipulator::generate(tinyxml2::XMLDocument& doc, tinyxml2::XMLNode * root)
{
	for (size_t i = 0; i < mMap.size(); i++)
	{
		auto ele_layer = doc.NewElement("layer");
		ele_layer->SetAttribute("id", static_cast<unsigned int>(i));
		mMap[i].generate_xml(ele_layer, doc);
		root->InsertEndChild(ele_layer);
	}
}

void tilemap_manipulator::generate(const std::string& pPath)
{
	tinyxml2::XMLDocument doc;
	auto root = doc.InsertEndChild(doc.NewElement("map"));
	generate(doc, root);
	doc.SaveFile(pPath.c_str());
}

size_t tilemap_manipulator::new_layer()
{
	mMap.emplace_back();
	return mMap.size() - 1;
}

size_t tilemap_manipulator::insert_layer(size_t pIndex)
{
	mMap.emplace(mMap.begin() + pIndex);
	return pIndex;
}

void tilemap_manipulator::remove_layer(size_t pIndex)
{
	assert(pIndex < mMap.size());
	mMap.erase(mMap.begin() + pIndex);
}

size_t tilemap_manipulator::get_layer_count() const
{
	return mMap.size();
}

tilemap_layer& tilemap_manipulator::get_layer(size_t pIndex)
{
	return mMap[pIndex];
}

bool tilemap_manipulator::move_layer(size_t pFrom, size_t pTo)
{
	if (pFrom == pTo)
		return true;

	tilemap_layer temp = std::move(mMap[pFrom]);
	mMap.insert(mMap.begin() + pTo, std::move(temp));
	if (pFrom > pTo)
		mMap.erase(mMap.begin() + pFrom + 1);
	else
		mMap.erase(mMap.begin() + pFrom);
	return true;
}

void tilemap_manipulator::clear()
{
	mMap.clear();
}

engine::fvector tilemap_manipulator::get_center_point() const
{
	if (mMap.empty())
		return{};

	engine::fvector sum;
	for (const auto& i : mMap)
		sum += i.get_center_point();
	return sum/static_cast<float>(mMap.size());
}

tile::tile()
{
	mFill = { 1, 1 };
	mRotation = 0;
}

tile::tile(tile && pMove)
{
	mPosition = pMove.mPosition;
	mFill = pMove.mFill;
	mRotation = pMove.mRotation;
	mAtlas_handle = std::move(pMove.mAtlas_handle);
}

tile::tile(const tile & pTile)
{
	mPosition = pTile.mPosition;
	mFill = pTile.mFill;
	mRotation = pTile.mRotation;
	mAtlas_handle = pTile.mAtlas_handle;
}

tile & tile::operator=(const tile & pTile)
{
	mPosition = pTile.mPosition;
	mFill = pTile.mFill;
	mRotation = pTile.mRotation;
	mAtlas_handle = pTile.mAtlas_handle;
	return *this;
}

bool tile::operator==(const tile & pRight)
{
	if (mPosition != pRight.mPosition)
		return false;
	if (mFill != pRight.mFill)
		return false;
	if (mRotation != pRight.mRotation)
		return false;
	if (mAtlas_handle != pRight.mAtlas_handle)
		return false;
	return true;
}

bool tile::operator!=(const tile & pRight)
{
	return !(*this == pRight);
}

void tile::set_position(engine::fvector pPosition)
{
	mPosition = pPosition;
}

engine::fvector tile::get_position() const
{
	return mPosition;
}

void tile::set_fill(fill_t pFill)
{
	assert(pFill.x >= 1);
	assert(pFill.y >= 1);
	mFill = pFill;
}

tile::fill_t tile::get_fill() const
{
	return mFill;
}

void tile::set_rotation(rotation_t pRotation)
{
	mRotation = pRotation%4;
}

tile::rotation_t tile::get_rotation() const
{
	return mRotation;
}

void tile::set_atlas(const std::string & pAtlas, tile_atlas_pool & pPool)
{
	mAtlas_handle = pPool.get(pAtlas);
}

void tile::set_atlas(tile_atlas_pool::handle pHandle)
{
	mAtlas_handle = pHandle;
}

const std::string& tile::get_atlas() const
{
	return *mAtlas_handle;
}

void tilemap_layer::set_name(const std::string & pName)
{
	mName = pName;
}

const std::string & tilemap_layer::get_name() const
{
	return mName;
}

tile* tilemap_layer::new_tile(const std::string & pAtlas)
{
	tile ntile;
	ntile.set_atlas(pAtlas, mAtlas_pool);
	mTiles.push_back(ntile);
	return &mTiles.back();
}

tile* tilemap_layer::set_tile(engine::fvector pPosition, engine::uvector pFill, const std::string & pAtlas, int pRotation)
{
	// Replace Tile
	if (tile* repl_tile = find_tile(pPosition))
	{
		repl_tile->set_fill(pFill);
		repl_tile->set_atlas(pAtlas, mAtlas_pool);
		repl_tile->set_rotation(pRotation);
		return repl_tile;
	}

	// Add a new one
	tile* ntile = new_tile(pAtlas);
	ntile->set_fill(pFill);
	ntile->set_position(pPosition);
	ntile->set_rotation(pRotation);
	return ntile;
}

tile* tilemap_layer::set_tile(engine::fvector pPosition, const std::string & pAtlas, int pRotation)
{
	return set_tile(pPosition, {1, 1}, pAtlas, pRotation);
}

tile* tilemap_layer::set_tile(const tile & pTile)
{
	return set_tile(pTile.get_position(), pTile.get_fill(), pTile.get_atlas(), pTile.get_rotation());
}

tile* tilemap_layer::find_tile(engine::fvector pPosition)
{
	for (auto& i : mTiles)
		if (i.get_position() == pPosition)
			return &i;
	return nullptr;
}

size_t tilemap_layer::get_tile_count() const
{
	return mTiles.size();
}

tile* tilemap_layer::get_tile(size_t pIndex)
{
	return &mTiles[pIndex];
}

const tile* tilemap_layer::get_tile(size_t pIndex) const
{
	return &mTiles[pIndex];
}

bool tilemap_layer::remove_tile(engine::fvector pPosition)
{
	for (size_t i = 0; i < mTiles.size(); i++)
		if (mTiles[i].get_position() == pPosition)
		{
			mTiles.erase(mTiles.begin() + i);
			return true;
		}
	return false;
}

float tilemap_layer::condense()
{
	// There needs to be at least 2 tiles to work with.
	if (mTiles.size() <= 1)
		return 1.f;

	size_t original_size = mTiles.size();

	// Sort the tiles, adjacent tiles will naturally be adjacent left to right in the array.
	std::sort(mTiles.begin(), mTiles.end(), [](const tile& l, const tile& r)
	{
		auto pos1 = l.get_position();
		auto pos2 = r.get_position();
		return (pos1.y < pos2.y) || ((pos1.y == pos2.y) && (pos1.x < pos2.x));
	});

	// Condense all tiles adjacent left and right.
	for (size_t i = 0; i < mTiles.size() - 1; i++)
	{
		if (mTiles[i].is_adjacent_left(mTiles[i + 1]))
		{
			mTiles[i].set_fill(mTiles[i].get_fill() + tile::fill_t(mTiles[i + 1].get_fill().x, 0));
			mTiles.erase(mTiles.begin() + i + 1);
			--i;
		}
	}

	// Sort the tiles, adjacent tiles will naturally be adjacent up to down in the array.
	std::sort(mTiles.begin(), mTiles.end(), [](const tile& l, const tile& r)
	{
		auto pos1 = l.get_position();
		auto pos2 = r.get_position();
		return (pos1.x < pos2.x) || ((pos1.x == pos2.x) && (pos1.y < pos2.y)); // x and y swapped
	});

	// Condense all tiles adjacent up and down
	for (size_t i = 0; i < mTiles.size() - 1; i++)
	{
		if (mTiles[i].is_adjacent_above(mTiles[i + 1]))
		{
			mTiles[i].set_fill(mTiles[i].get_fill() + tile::fill_t(0, mTiles[i + 1].get_fill().y));
			mTiles.erase(mTiles.begin() + i + 1);
			--i;
		}
	}

	// Return [New size]/[Old size]
	return static_cast<float>(mTiles.size()) / static_cast<float>(original_size);
}

void tilemap_layer::explode_tile(tile * pTile)
{
	assert(pTile);
	tile cp_tile = *pTile; // Modifying mTiles invalidates the pointer
	pTile->set_fill({ 1, 1 });
	for (int x = 0; x < cp_tile.get_fill().x; x++)
	{
		for (int y = 0; y < cp_tile.get_fill().y; y++)
		{
			if (x == 0 && y == 0) continue; // This is the main tile. Skip it. It's already there.
			tile ntile = cp_tile;
			ntile.set_fill({ 1, 1 });
			ntile.set_position(cp_tile.get_position() + engine::fvector(x, y));
			mTiles.push_back(ntile);
		}
	}
}

void tilemap_layer::explode()
{
	for (size_t i = 0; i < mTiles.size(); i++)
		if (mTiles[i].is_condensed())
			explode_tile(&mTiles[i]);
}

bool tilemap_layer::load_xml(tinyxml2::XMLElement * pRoot)
{
	mName = util::safe_string(pRoot->Attribute("name"));

	auto i = pRoot->FirstChildElement();
	while (i)
	{
		tile ntile;
		ntile.load_xml(i, mAtlas_pool);
		mTiles.push_back(ntile);
		i = i->NextSiblingElement();
	}
	sort();
	return true;
}

void tilemap_layer::generate_xml(tinyxml2::XMLElement * pRoot, tinyxml2::XMLDocument & doc) const
{
	pRoot->SetAttribute("name", mName.c_str());
	for (auto &i : mTiles)
	{
		auto ele = doc.NewElement(i.get_atlas().c_str());
		ele->SetAttribute("x", i.get_position().x);
		ele->SetAttribute("y", i.get_position().y);
		if (i.get_fill().x > 1)    ele->SetAttribute("w", i.get_fill().x);
		if (i.get_fill().y > 1)    ele->SetAttribute("h", i.get_fill().y);
		if (i.get_rotation() != 0) ele->SetAttribute("r", i.get_rotation());
		pRoot->InsertEndChild(ele);
	}
}

tile_atlas_pool & tilemap_layer::get_pool()
{
	return mAtlas_pool;
}

engine::fvector tilemap_layer::get_center_point() const
{
	if (mTiles.empty())
		return{};
	engine::fvector sum;
	for (const auto& i : mTiles)
		sum += i.get_position();
	return sum/static_cast<float>(mTiles.size());
}

void tilemap_layer::sort()
{
	std::sort(mTiles.begin(), mTiles.end(),
		[](const tile& l, const tile& r)
		{
			return l.get_position() < r.get_position(); 
		});
}

tile_atlas_pool::handle tile_atlas_pool::get(const std::string & pAtlas)
{
	auto item = std::find_if(mStrings.begin(), mStrings.end(),
		[&](const std::shared_ptr<std::string>& a)->bool
	{
		return *a == pAtlas;
	});

	// Create a new item if it doesn't exist in this pool
	if (item == mStrings.end())
	{
		auto nitem = std::make_shared<std::string>(pAtlas);
		mStrings.push_back(nitem);
		return nitem;
	}
	return *item;
}

bool tile_atlas_pool::replace(const std::string & pOriginal, const std::string & pNew)
{
	auto item = std::find_if(mStrings.begin(), mStrings.end(),
		[&](const std::shared_ptr<std::string>& a)->bool
		{
			return *a == pOriginal;
		});
	if (item == mStrings.end())
		return false;

	(*item)->assign(pNew);
	return true;
}

std::vector<std::string> tile_atlas_pool::get_invalid_entries(std::shared_ptr<engine::texture> pTexture) const
{
	std::vector<std::string> inval_entrs;
	for (auto& i : mStrings)
		if (!pTexture->get_entry(*i))
			inval_entrs.push_back(*i);
	return inval_entrs;
}

bool tile_atlas_pool::has_invalid_entries(std::shared_ptr<engine::texture> pTexture) const
{
	return !get_invalid_entries(pTexture).empty();
}
