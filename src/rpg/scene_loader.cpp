#include <rpg/scene_loader.hpp>

#include <engine/utility.hpp>

#include <filesystem>
namespace fs = std::experimental::filesystem;

using namespace rpg;

scene_loader::scene_loader()
{
	mEle_collisionboxes = nullptr;
	mEle_map = nullptr;
}

int scene_loader::load(const std::string & pName)
{
	mXml_Document.Clear();
	fs::path scene_directory = fs::current_path()
		/ "data/scenes";

	fs::path scene_path = scene_directory
		/ (pName + ".xml");
	mScene_path = scene_path.string();

	fs::path script_path = scene_directory
		/ (pName + ".as");
	mScript_path = script_path.string();

	if (mXml_Document.LoadFile(mScene_path.c_str()))
	{
		util::error("Unable to open scene. Please check path.");
		return 1;
	}

	auto ele_root = mXml_Document.FirstChildElement("scene");
	if (!ele_root)
	{
		util::error("Unable to get root element 'scene'.");
		return 1;
	}

	// Get collision boxes
	mEle_collisionboxes = ele_root->FirstChildElement("collisionboxes");

	// Get boundary
	if (auto ele_boundary = ele_root->FirstChildElement("boundary"))
	{
		engine::frect boundary;
		boundary.x = ele_boundary->FloatAttribute("x");
		boundary.y = ele_boundary->FloatAttribute("y");
		boundary.w = ele_boundary->FloatAttribute("w");
		boundary.h = ele_boundary->FloatAttribute("h");
		mBoundary = boundary * 32.f;
		mHas_boundary = true;
	}
	else
	{
		mHas_boundary = false;
	}

	// Get tilemap
	if (mEle_map = ele_root->FirstChildElement("map"))
	{
		// Load tilemap texture
		if (auto ele_texture = mEle_map->FirstChildElement("texture"))
		{
			mTilemap_texture = ele_texture->GetText();
		}
		else
		{
			util::error("Tilemap texture is not defined");
			return 1;
		}
	}

	return 0;
}

void scene_loader::clean()
{
	mXml_Document.Clear();
	mScript_path.clear();
	mScene_name.clear();
	mTilemap_texture.clear();
	mScene_path.clear();
	mBoundary = engine::frect();
	mHas_boundary = false;
	mEle_collisionboxes = nullptr;
	mEle_map = nullptr;
}

bool scene_loader::has_boundary()
{
	return mHas_boundary;
}

const engine::frect& scene_loader::get_boundary()
{
	return mBoundary;
}

const std::string& scene_loader::get_name()
{
	return mScene_name;
}

const std::string& scene_loader::get_script_path()
{
	return mScript_path;
}

const std::string& scene_loader::get_tilemap_texture()
{
	return mTilemap_texture;
}

const std::string & rpg::scene_loader::get_scene_path()
{
	return mScene_path;
}

std::vector<engine::frect> scene_loader::construct_wall_list()
{
	assert(mEle_collisionboxes != 0);

	std::vector<engine::frect> walls;

	auto ele_wall = mEle_collisionboxes->FirstChildElement("wall");
	while (ele_wall)
	{
		walls.push_back(util::shortcuts::rect_float_att(ele_wall));
		ele_wall = ele_wall->NextSiblingElement("wall");
	}

	return walls;
}

util::optional_pointer<tinyxml2::XMLElement> scene_loader::get_collisionboxes()
{
	return mEle_collisionboxes;
}

util::optional_pointer<tinyxml2::XMLElement> scene_loader::get_tilemap()
{
	return mEle_map;
}

tinyxml2::XMLDocument& rpg::scene_loader::get_document()
{
	return mXml_Document;
}
