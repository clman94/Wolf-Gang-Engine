#include <rpg/scene_loader.hpp>

#include <engine/utility.hpp>

using namespace rpg;

scene_loader::scene_loader()
{
	mEle_collisionboxes = nullptr;
	mEle_map = nullptr;
}

int scene_loader::load(const std::string & pPath)
{
	mXml_Document.Clear();
	if (mXml_Document.LoadFile(pPath.c_str()))
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

	// Get scene script path
	if (auto ele_script = ele_root->FirstChildElement("script"))
	{
		auto path = ele_script->Attribute("path");
		if (!path)
		{
			util::error("Please specify the path of the script");
			return 1;
		}
		mScript_path = path;
	}

	// Get boundary
	if (auto ele_boundary = ele_root->FirstChildElement("boundary"))
	{
		engine::fvector boundary;
		boundary.x = ele_boundary->FloatAttribute("w");
		boundary.y = ele_boundary->FloatAttribute("h");
		mBoundary = boundary * 32;
	}
	else
	{
		util::error("Please specify boundary of tilemap");
		return 1;
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
	mScene_path = pPath;

	return 0;
}

const engine::fvector& scene_loader::get_boundary()
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

tinyxml2::XMLElement* scene_loader::get_collisionboxes()
{
	return mEle_collisionboxes;
}

tinyxml2::XMLElement* scene_loader::get_tilemap()
{
	return mEle_map;
}

tinyxml2::XMLDocument& rpg::scene_loader::get_document()
{
	return mXml_Document;
}
