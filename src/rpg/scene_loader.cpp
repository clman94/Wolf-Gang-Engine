#include <rpg/scene_loader.hpp>
#include <rpg/rpg_config.hpp>

#include <engine/logger.hpp>
#include <algorithm>

using namespace rpg;

std::vector<engine::encoded_path> rpg::get_scene_list()
{
	std::vector<engine::encoded_path> ret;

	const engine::encoded_path dir = (defs::DEFAULT_DATA_PATH / defs::DEFAULT_SCENES_PATH).string();
	for (auto& i : engine::fs::recursive_directory_iterator(defs::DEFAULT_DATA_PATH / defs::DEFAULT_SCENES_PATH))
	{
		engine::encoded_path path = i.path().string();
		if (path.extension() == ".xml")
		{
			path.snip_path(dir);
			path.remove_extension();
			ret.push_back(path);
		}
	}

	std::sort(ret.begin(), ret.end());
	return ret;
}

scene_loader::scene_loader()
{
	mEle_collisionboxes = nullptr;
	mEle_map = nullptr;
}

bool scene_loader::load(const engine::encoded_path& pDir, const std::string & pName)
{
	clean();
	
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

bool scene_loader::load(const engine::encoded_path& pDir, const std::string & pName, engine::pack_stream_factory& pPack)
{
	clean();

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
	return !mXml_Document.SaveFile(mScene_path.string().c_str());
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

bool scene_loader::load_settings()
{
	fix();

	auto ele_root = mXml_Document.FirstChildElement("scene");
	if (!ele_root)
	{
		logger::error("Unable to get root element 'scene'.");
		return false;
	}

	// Get collision boxes
	mEle_collisionboxes = ele_root->FirstChildElement("collisionboxes");

	// Get boundary
	if (auto ele_boundary = ele_root->FirstChildElement("boundary"))
	{
		mBoundary.x = ele_boundary->FloatAttribute("x");
		mBoundary.y = ele_boundary->FloatAttribute("y");
		mBoundary.w = ele_boundary->FloatAttribute("w");
		mBoundary.h = ele_boundary->FloatAttribute("h");
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
			mTilemap_texture = util::safe_string(ele_texture->GetText());
		else
			logger::warning("Tilemap texture is not defined");

	}
	return true;
}

void scene_loader::fix()
{
	if (mXml_Document.Error())
		mXml_Document.Clear();

	auto ele_scene = mXml_Document.FirstChildElement("scene");
	if (!ele_scene)
	{
		logger::info("Fixing missing 'scene' element");
		ele_scene = mXml_Document.NewElement("scene");
		mXml_Document.InsertFirstChild(ele_scene);
	}

	auto ele_map = ele_scene->FirstChildElement("map");
	if (!ele_map)
	{
		logger::info("Fixing missing 'map' element");
		ele_map = mXml_Document.NewElement("map");
		ele_scene->InsertFirstChild(ele_map);
	}

	auto ele_texture = ele_map->FirstChildElement("texture");
	if (!ele_texture)
	{
		logger::info("Fixing missing 'texture' element (The texture may need to be specified for tilemaps to work)");
		ele_map->InsertFirstChild(mXml_Document.NewElement("texture"));
	}

	auto ele_collisionboxes = ele_scene->FirstChildElement("collisionboxes");
	if (!ele_collisionboxes)
	{
		logger::info("Fixing missing 'collisionboxes' element");
		ele_scene->InsertFirstChild(mXml_Document.NewElement("collisionboxes"));
	}
}

bool scene_loader::has_boundary() const
{
	return mHas_boundary;
}

const engine::frect& scene_loader::get_boundary() const
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
	return mTilemap_texture.string();
}

std::string scene_loader::get_scene_path() const
{
	return mScene_path.string();
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
