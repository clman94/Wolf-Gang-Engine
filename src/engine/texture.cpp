#define ENGINE_INTERNAL

#include <engine/texture.hpp>

#include "../../tinyxml2/tinyxml2.h"
#include "../xmlshortcuts.hpp"

#include <iostream>

using namespace engine;

atlas_entry::atlas_entry()
{
	mAnimation = std::make_shared<animation>();
}

atlas_entry::atlas_entry(std::shared_ptr<animation> pAnimation)
{
	mAnimation = pAnimation;
}

frect atlas_entry::get_root_rect() const
{
	return mAnimation->get_frame_at(0);
}

bool atlas_entry::is_animation() const
{
	return mAnimation->get_frame_count() > 1 && mAnimation->get_interval() > 0;
}

std::shared_ptr<engine::animation> atlas_entry::get_animation() const
{
	return mAnimation;
}
bool atlas_entry::load(tinyxml2::XMLElement * pEle)
{
	assert(pEle != nullptr);

	// Set root frame
	mAnimation->set_frame_rect(util::shortcuts::load_rect_float_att(pEle));

	// Set frame count
	int att_frames = pEle->IntAttribute("frames");
	engine::frame_t frame_count = (att_frames <= 0 ? 1 : att_frames);// Default one frame
	mAnimation->set_frame_count(frame_count);

	// Set starting interval
	float att_interval = pEle->FloatAttribute("interval");
	mAnimation->add_interval(0, att_interval);

	// Set default frame (default : 0)
	int att_default = pEle->IntAttribute("default");
	mAnimation->set_default_frame(att_default);

	// Set loop type (default : none)
	bool att_loop = pEle->BoolAttribute("loop");
	bool att_pingpong = pEle->BoolAttribute("pingpong");

	engine::animation::loop_type loop_type = engine::animation::loop_type::none;
	if (att_loop)                loop_type = engine::animation::loop_type::linear;
	if (att_pingpong)            loop_type = engine::animation::loop_type::pingpong;
	mAnimation->set_loop(loop_type);

	// Setup sequence for changing of interval over time
	auto ele_seq = pEle->FirstChildElement("seq");
	while (ele_seq)
	{
		mAnimation->add_interval(
			(engine::frame_t)ele_seq->IntAttribute("from"),
			ele_seq->FloatAttribute("interval"));
		ele_seq = ele_seq->NextSiblingElement();
	}
	return true;
}

bool atlas_entry::save(tinyxml2::XMLElement * pEle)
{
	util::shortcuts::save_rect_float_att(pEle, mAnimation->get_frame_at(0));
	if (mAnimation->get_frame_count() > 1)
		pEle->SetAttribute("frames", static_cast<unsigned int>(mAnimation->get_frame_count()));
	if (mAnimation->get_interval() > 0)
		pEle->SetAttribute("interval", mAnimation->get_interval());
	switch (mAnimation->get_loop())
	{
	case engine::animation::loop_type::linear:
		pEle->SetAttribute("loop", 1);
		break;
	case engine::animation::loop_type::pingpong:
		pEle->SetAttribute("pingpong", 1);
		break;
	default:
		break;
	}

	if (mAnimation->get_default_frame() != 0)
		pEle->SetAttribute("default", static_cast<unsigned int>(mAnimation->get_default_frame()));

	// TODO: Save sequenced interval
	return true;
}

bool texture_atlas::load(const std::string & pPath)
{
	clean();

	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
		return false;
	return load_settings(doc);
}

bool texture_atlas::save(const std::string & pPath)
{
	using namespace tinyxml2;

	XMLDocument doc;
	auto root = doc.NewElement("atlas");
	doc.InsertEndChild(root);
	
	for (auto& i : mAtlas)
	{
		auto entry = doc.NewElement(i.first.c_str());
		i.second.save(entry);
		root->InsertEndChild(entry);
	}
	return doc.SaveFile(pPath.c_str()) == XML_SUCCESS;
}

bool texture_atlas::load_memory(const char * pData, size_t pSize)
{
	clean();

	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.Parse(pData, pSize))
		return false;
	return load_settings(doc);
}

void texture_atlas::clean()
{
	mAtlas.clear();
}

util::optional_pointer<const atlas_entry> texture_atlas::get_entry(const std::string & pName) const
{
	auto find_entry = mAtlas.find(pName);
	if (find_entry == mAtlas.end())
		return{};
	return &find_entry->second;
}

util::optional_pointer<const std::pair<const std::string, atlas_entry>> texture_atlas::get_entry(const fvector & pVec) const
{
	for (auto& i : mAtlas)
	{
		if (i.second.get_root_rect().is_intersect(pVec))
			return &i;
	}
	return{};
}

bool engine::texture_atlas::add_entry(const std::string & pName, const atlas_entry & pEntry)
{
	if (mAtlas.find(pName) != mAtlas.end())
		return false;
	mAtlas[pName] = pEntry;
	return true;
}

bool texture_atlas::rename_entry(const std::string & pOriginal, const std::string & pRename)
{
	if (mAtlas.find(pRename) != mAtlas.end())
		return false;
	mAtlas[pRename] = mAtlas[pOriginal];
	mAtlas.erase(pOriginal);
	return true;
}

bool texture_atlas::remove_entry(const std::string & pName)
{
	if (mAtlas.find(pName) != mAtlas.end())
		return false;
	mAtlas.erase(pName);
	return true;
}

std::vector<std::string> texture_atlas::compile_list() const
{
	std::vector<std::string> list;
	for (auto& i : mAtlas)
		list.push_back(i.first);
	return std::move(list);
}

const std::map<std::string, atlas_entry>& engine::texture_atlas::get_raw_atlas() const
{
	return mAtlas;
}


bool texture_atlas::load_settings(tinyxml2::XMLDocument& pDoc)
{
	auto ele_atlas = pDoc.FirstChildElement("atlas");
	if (!ele_atlas)
		return false;

	auto ele_entry = ele_atlas->FirstChildElement();
	while (ele_entry)
	{
		// TODO: Check for colliding names
		mAtlas[ele_entry->Name()].load(ele_entry);

		ele_entry = ele_entry->NextSiblingElement();
	}
	return true;
}

void texture::set_texture_source(const std::string& pFilepath)
{
	mTexture_source = pFilepath;
}

void texture::set_atlas_source(const std::string & pFilepath)
{
	mAtlas_source = pFilepath;
}

bool texture::load()
{
	if (!is_loaded())
	{
		mSFML_texture.reset(new sf::Texture());
		if (mPack)
		{
			{ // Texture
				auto data = mPack->read_all(mTexture_source);
				set_loaded(mSFML_texture->loadFromMemory(&data[0], data.size()));
			}
			if (!mAtlas_source.empty())
			{ // Atlas
				auto data = mPack->read_all(mAtlas_source);
				mAtlas.load_memory(&data[0], data.size());
			}
		}
		else
		{
			set_loaded(mSFML_texture->loadFromFile(mTexture_source));
			if (!mAtlas_source.empty())
				mAtlas.load(mAtlas_source);
		}
	}
	return is_loaded();
}

bool texture::unload()
{
	mSFML_texture.reset();
	set_loaded(false);
	return true;
}

util::optional_pointer<const atlas_entry> texture::get_entry(const std::string & pName) const
{
	return mAtlas.get_entry(pName);
}

std::vector<std::string> engine::texture::compile_list() const
{
	return mAtlas.compile_list();
}

fvector texture::get_size() const
{
	return{ static_cast<float>(mSFML_texture->getSize().x), static_cast<float>(mSFML_texture->getSize().y) };
}
