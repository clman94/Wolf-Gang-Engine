#include <rpg/scene_loader.hpp>
#include <rpg/rpg_config.hpp>
#include <rpg/tilemap_manipulator.hpp>
#include <rpg/collision_box.hpp>

#include <engine/logger.hpp>
#include <engine/utility.hpp>
#include <algorithm>

using namespace rpg;


scene_loader::scene_loader()
{
	clear();
}

bool scene_loader::load(const engine::generic_path& pDir, const std::string & pName)
{
	clear();
	
	mScene_path = pDir / (pName + ".xml");
	mScript_path = pDir / (pName + ".as");
	mScene_name = pName;

	if (mXml_Document.LoadFile(mScene_path.string().c_str()))
	{
		logger::error("Unable to open scene XML file.");
		return false;
	}

	return load_settings();
}

bool scene_loader::load(const engine::generic_path& pDir, const std::string & pName, engine::resource_pack& pPack)
{
	clear();

	mScene_path = pDir / (pName + ".xml");
	mScript_path = pDir / (pName + ".as");
	mScene_name = pName;

	auto data = pPack.read_all(mScene_path.string());
	if (data.empty())
		return false;

	if (mXml_Document.Parse(&data[0], data.size()))
	{
		logger::error("Unable to open scene XML file.");
		return false;
	}

	return load_settings();
}

bool scene_loader::save()
{
	save_settings();
	return !mXml_Document.SaveFile(mScene_path.string().c_str());
}

void scene_loader::clear()
{
	mXml_Document.Clear();
	mScript_path.clear();
	mScene_name.clear();
	mScene_path.clear();
	mEle_collisionboxes = nullptr;
	mEle_map = nullptr;
	mEle_root = nullptr;
	mHas_boundary = false;
	mBoundary = { 0, 0, 0, 0 };
	mTilemap_texture.clear();
}

bool scene_loader::load_settings()
{
	fix();

	mEle_root = mXml_Document.FirstChildElement("scene");
	if (!mEle_root)
	{
		logger::error("Unable to get root element 'scene'.");
		return false;
	}

	auto ele_has_boundary = mEle_root->FirstChildElement("has_boundary");
	assert(ele_has_boundary);
	mHas_boundary = ele_has_boundary->BoolText(false);

	auto ele_boundary = mEle_root->FirstChildElement("boundary");
	assert(ele_boundary);
	mBoundary.x = ele_boundary->FloatAttribute("x");
	mBoundary.y = ele_boundary->FloatAttribute("y");
	mBoundary.w = ele_boundary->FloatAttribute("w");
	mBoundary.h = ele_boundary->FloatAttribute("h");

	mEle_map = mEle_root->FirstChildElement("map");

	auto ele_texture = mEle_map->FirstChildElement("texture");
	assert(ele_texture);
	mTilemap_texture = util::safe_string(ele_texture->GetText());

	mEle_collisionboxes = mEle_root->FirstChildElement("collisionboxes");

	return true;
}

void scene_loader::save_settings()
{
	fix();

	assert(mEle_root);

	auto ele_boundary = mEle_root->FirstChildElement("boundary");
	assert(ele_boundary);
	ele_boundary->SetAttribute("x", mBoundary.x);
	ele_boundary->SetAttribute("y", mBoundary.y);
	ele_boundary->SetAttribute("w", mBoundary.w);
	ele_boundary->SetAttribute("h", mBoundary.h);

	auto ele_has_boundary = mEle_root->FirstChildElement("hasBoundary");
	assert(ele_has_boundary);
	ele_has_boundary->SetText(mHas_boundary);

	assert(mEle_map);
	auto ele_texture = mEle_map->FirstChildElement("texture");
	assert(ele_texture);
	ele_texture->SetText(mTilemap_texture.c_str());
}

// Returns true if element did not exist. pNew_ele remains unchanged if it does exist.
static inline bool ensure_xml_ele_exists(tinyxml2::XMLDocument& pDoc, tinyxml2::XMLElement* pParent, const char* pName, tinyxml2::XMLElement** pNew_ele = nullptr)
{
	assert(pParent);
	assert(pName);
	auto ele = pParent->FirstChildElement(pName);
	if (!ele)
	{
		ele = pDoc.NewElement(pName);
		pParent->InsertEndChild(ele);
		if (pNew_ele)
			*pNew_ele = ele;
		return true;
	}
	return false;
}

void scene_loader::fix()
{
	if (mXml_Document.Error())
		mXml_Document.Clear();

	mEle_root = mXml_Document.FirstChildElement("scene");
	if (!mEle_root)
	{
		mEle_root = mXml_Document.NewElement("scene");
		mXml_Document.InsertFirstChild(mEle_root);
	}

	tinyxml2::XMLElement* ele_has_boundary;
	if (ensure_xml_ele_exists(mXml_Document, mEle_root, "has_boundary", &ele_has_boundary))
		ele_has_boundary->SetText(false);

	tinyxml2::XMLElement* ele_boundary;
	if (ensure_xml_ele_exists(mXml_Document, mEle_root, "boundary", &ele_boundary))
	{
		ele_boundary->SetAttribute("x", 0);
		ele_boundary->SetAttribute("y", 0);
		ele_boundary->SetAttribute("w", 0);
		ele_boundary->SetAttribute("h", 0);
	}

	ensure_xml_ele_exists(mXml_Document, mEle_root, "map");
	ensure_xml_ele_exists(mXml_Document, mEle_root->FirstChildElement("map"), "texture");
	ensure_xml_ele_exists(mXml_Document, mEle_root, "collisionboxes");
}

void scene_loader::set_has_boundary(bool pHas_it)
{
	mHas_boundary = pHas_it;
}

bool scene_loader::has_boundary() const
{
	return mHas_boundary;
}

void scene_loader::set_boundary(const engine::frect & pRect)
{
	mBoundary = pRect;
}

engine::frect scene_loader::get_boundary() const
{
	return mBoundary;
}

const std::string& scene_loader::get_name()
{
	return mScene_name;
}

std::string scene_loader::get_script_path() const
{
	return mScript_path.string();
}

std::string scene_loader::get_tilemap_texture() const
{
	return mTilemap_texture;
}

void scene_loader::set_tilemap_texture(const std::string& pName)
{
	mTilemap_texture = pName;
}

std::string scene_loader::get_scene_path() const
{
	return mScene_path.string();
}

tinyxml2::XMLElement* scene_loader::get_collisionboxes()
{
	return mEle_collisionboxes;
}

void scene_loader::set_collisionboxes(const collision_box_container & pCBC)
{
	mEle_collisionboxes->DeleteChildren();
	pCBC.save_xml(mXml_Document, mEle_collisionboxes);
}

tinyxml2::XMLElement* scene_loader::get_tilemap()
{
	return mEle_map;
}

void scene_loader::set_tilemap(const tilemap_manipulator & pTm_m)
{
	mEle_map->DeleteChildren();
	pTm_m.save_xml(mXml_Document, mEle_map);
}

tinyxml2::XMLDocument& scene_loader::get_document()
{
	return mXml_Document;
}
