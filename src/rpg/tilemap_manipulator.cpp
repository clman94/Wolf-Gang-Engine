#include <rpg/tilemap_manipulator.hpp>
#include <rpg/rpg_config.hpp>

using namespace rpg;

void
tilemap_manipulator::tile::load_xml(tinyxml2::XMLElement * pEle)
{
	assert(pEle != nullptr);

	mAtlas = util::safe_string(pEle->Name());

	mPosition.x = pEle->FloatAttribute("x");
	mPosition.y = pEle->FloatAttribute("y");

	mFill.x = pEle->UnsignedAttribute("w");
	mFill.y = pEle->UnsignedAttribute("h");

	// Default fill to 1 for 1x1 tile
	mFill.x = (mFill.x == 0 ? 1 : mFill.x);
	mFill.y = (mFill.y == 0 ? 1 : mFill.y);

	mRotation = pEle->UnsignedAttribute("r") % 4;
}

bool
tilemap_manipulator::tile::is_adjacent_above(tile & a)
{
	return (
		mAtlas == a.mAtlas
		&& mPosition.x == a.mPosition.x
		&& mPosition.y == a.mPosition.y + a.mFill.y
		&& mFill.x == a.mFill.x
		&& mRotation == a.mRotation
		);
}

bool
tilemap_manipulator::tile::is_adjacent_right(tile & a)
{
	return (
		mAtlas == a.mAtlas
		&& mPosition.y == a.mPosition.y
		&& mPosition.x + static_cast<float>(mFill.x) == a.mPosition.x
		&& mFill.y == a.mFill.y
		&& mRotation == a.mRotation
		);
}

bool tilemap_manipulator::tile::operator<(const tile & pTile)
{
	return mPosition < pTile.mPosition;
}

tilemap_manipulator::tile*
tilemap_manipulator::find_tile(engine::fvector pos, int layer)
{
	for (auto &i : mMap[layer])
	{
		if (i.second.get_position() == pos)
			return &i.second;
	}
	return nullptr;
}

// Uses brute force to merge adjacent tiles (still a little wonky)
void
tilemap_manipulator::condense_layer(layer &pMap)
{
	// 2 or more tiles present are required
	if (pMap.size() < 2)
		return;

	layer nmap;

	tile new_tile = pMap.begin()->second;

	bool merged = false;
	for (auto i = pMap.begin(); i != pMap.end(); i++)
	{
		auto& current_tile = i->second;

		// Merge adjacent tile
		if (new_tile.is_adjacent_right(current_tile))
		{
			auto fill = new_tile.get_fill();
			fill.x += current_tile.get_fill().x;
			new_tile.set_fill(fill);
		}
		else // No more tiles to the right
		{
			// Merge ntime to any tile above
			for (auto &j : nmap)
			{
				if (new_tile.is_adjacent_above(j.second))
				{
					auto fill = j.second.get_fill();
					fill.y += new_tile.get_fill().y;
					j.second.set_fill(fill);
					merged = true;
					break; //
				}
			}
			if (!merged) // Do not add when it was merged to another tile
			{
				nmap[new_tile.get_position()] = new_tile;
			}
			merged = false;
			new_tile = current_tile;
		}
	}
	nmap[new_tile.get_position()] = new_tile; // add last tile
	pMap = std::move(nmap);
}

void
tilemap_manipulator::condense_tiles()
{
	if (!mMap.size()) return;
	for (auto &i : mMap)
	{
		condense_layer(i.second);
	}
}

int
tilemap_manipulator::load_layer(tinyxml2::XMLElement * pEle, int pLayer)
{
	auto i = pEle->FirstChildElement();
	while (i)
	{
		tile ntile;
		ntile.load_xml(i);
		mMap[pLayer][ntile.get_position()] = ntile;
		i = i->NextSiblingElement();
	}
	return 0;
}

tilemap_manipulator::tilemap_manipulator()
{
	mTile_size = defs::TILE_SIZE;
}

int
tilemap_manipulator::load_tilemap_xml(tinyxml2::XMLElement *pRoot)
{
	clean();

	if (auto att_path = pRoot->Attribute("path"))
	{
		load_tilemap_xml(util::safe_string(att_path));
	}

	auto ele_tilemap = pRoot->FirstChildElement("layer");
	while (ele_tilemap)
	{
		int att_layer = ele_tilemap->IntAttribute("id");
		load_layer(ele_tilemap, att_layer);
		ele_tilemap = ele_tilemap->NextSiblingElement("layer");
	}
	return 0;
}

int tilemap_manipulator::load_tilemap_xml(std::string pPath)
{
	using namespace tinyxml2;
	XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
	{
		util::error("Error loading tilemap file");
		return 1;
	}
	auto root = doc.RootElement();
	load_tilemap_xml(root);
	return 0;
}

tilemap_manipulator::tile* tilemap_manipulator::find_tile_at(engine::fvector pPosition, int pLayer)
{
	for (auto &i : mMap[pLayer])
	{
		auto& tile = i.second;
		if (engine::frect(tile.get_position(), tile.get_fill()).is_intersect(pPosition))
		{
			return &tile;
		}
	}
	return nullptr;
}

void tilemap_manipulator::explode_tile(tile* pTile, int pLayer)
{
	auto atlas = pTile->get_atlas();
	auto fill = pTile->get_fill();
	auto rotation = pTile->get_rotation();
	auto position = pTile->get_position();
	pTile->set_fill({ 1, 1 });
	for (float x = 0; x < fill.x; x += 1)
		for (float y = 0; y < fill.y; y += 1)
			set_tile(position + engine::fvector(x, y), pLayer, atlas, rotation);
}

void tilemap_manipulator::explode_tile(engine::fvector pPosition, int pLayer)
{
	auto t = find_tile_at(pPosition, pLayer);
	if (!t || t->get_fill() == tile::fill_t(1, 1))
		return;
	explode_tile(t, pLayer);
}

void tilemap_manipulator::explode_all()
{
	for (auto &l : mMap)
		for (auto& i : l.second)
			explode_tile(&i.second, l.first);
}

void tilemap_manipulator::generate(tinyxml2::XMLDocument& doc, tinyxml2::XMLNode * root)
{
	std::map<int, tinyxml2::XMLElement *> layers;

	for (auto &l : mMap)
	{
		if (!l.second.size())
			continue;

		auto ele_layer = doc.NewElement("layer");
		ele_layer->SetAttribute("id", l.first);
		root->InsertEndChild(ele_layer);
		layers[l.first] = ele_layer;

		for (auto &i : l.second)
		{
			auto& tile = i.second;
			auto ele = doc.NewElement(tile.get_atlas().c_str());
			ele->SetAttribute("x", tile.get_position().x);
			ele->SetAttribute("y", tile.get_position().y);
			if (tile.get_fill().x > 1)    ele->SetAttribute("w", tile.get_fill().x);
			if (tile.get_fill().y > 1)    ele->SetAttribute("h", tile.get_fill().y);
			if (tile.get_rotation() != 0) ele->SetAttribute("r", tile.get_rotation());
			layers[l.first]->InsertEndChild(ele);
		}
	}
}

void tilemap_manipulator::generate(const std::string& pPath)
{
	using namespace tinyxml2;
	XMLDocument doc;
	auto root = doc.InsertEndChild(doc.NewElement("map"));
	generate(doc, root);
	doc.SaveFile(pPath.c_str());
}

int tilemap_manipulator::set_tile(engine::fvector pPosition, engine::fvector pFill, int pLayer, const std::string& pAtlas, int pRotation)
{
	engine::fvector off(0, 0);
	for (off.y = 0; off.y <= pFill.y; off.y++)
	{
		for (off.x = 0; off.x <= pFill.x; off.x++)
		{
			set_tile(pPosition + off, pLayer, pAtlas, pRotation);
		}
	}
	return 0;
}

int tilemap_manipulator::set_tile(engine::fvector pPosition, int pLayer, const std::string& pAtlas, int pRotation)
{
	auto &nt = mMap[pLayer][pPosition];
	nt.set_position(pPosition);
	nt.set_fill({ 1, 1 });
	nt.set_atlas(pAtlas);
	nt.set_rotation(pRotation);
	return 0;
}

void tilemap_manipulator::remove_tile(engine::fvector pPosition, int pLayer)
{
	for (auto i = mMap[pLayer].begin(); i != mMap[pLayer].end(); i++)
	{
		if (i->second.get_position() == pPosition)
		{
			mMap[pLayer].erase(i);
			break;
		}
	}
}

std::string tilemap_manipulator::find_tile_name(engine::fvector pPosition, int pLayer)
{
	auto f = find_tile_at(pPosition, pLayer);
	if (!f)
		return std::string();
	return f->get_atlas();
}

void tilemap_manipulator::update_display(tilemap_display& tmA)
{
	tmA.clean();
	for (auto &l : mMap) // Current layer
	{
		for (auto &i : l.second) // Tile
		{
			auto& tile = i.second;

			// Fill an area with a tile (be it 1x1 or other larger sizes)
			engine::fvector off(0, 0);
			for (off.y = 0; off.y < static_cast<float>(tile.get_fill().y); off.y++)
			{
				for (off.x = 0; off.x < static_cast<float>(tile.get_fill().x); off.x++)
				{
					tmA.set_tile((tile.get_position() + off)*mTile_size, tile.get_atlas(), l.first, tile.get_rotation());
				}
			}
		}
	}
}

void tilemap_manipulator::clean()
{
	mMap.clear();
}

void tilemap_manipulator::tile::set_position(engine::fvector pPosition)
{
	mPosition = pPosition;
}

engine::fvector tilemap_manipulator::tile::get_position() const
{
	return mPosition;
}

void tilemap_manipulator::tile::set_fill(fill_t pFill)
{
	mFill = pFill;
}

tilemap_manipulator::tile::fill_t tilemap_manipulator::tile::get_fill() const
{
	return mFill;
}

void tilemap_manipulator::tile::set_rotation(rotation_t pRotation)
{
	mRotation = pRotation;
}

tilemap_manipulator::tile::rotation_t tilemap_manipulator::tile::get_rotation() const
{
	return mRotation;
}

void tilemap_manipulator::tile::set_atlas(const std::string & pAtlas)
{
	mAtlas = pAtlas;
}

const std::string& tilemap_manipulator::tile::get_atlas() const
{
	return mAtlas;
}
