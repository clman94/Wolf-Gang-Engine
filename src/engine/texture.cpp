#define ENGINE_INTERNAL

#include <engine/texture.hpp>
#include <engine/logger.hpp>

#include "../../3rdparty/tinyxml2/tinyxml2.h"

#include <iostream>

using namespace engine;

subtexture::subtexture(const std::string & pName)
{
	mName = pName;
}

void subtexture::set_name(const std::string & pName)
{
	mName = pName;
	mHash = hash::hash32(pName);
}

const std::string & subtexture::get_name() const
{
	return mName;
}

hash::hash32_t subtexture::get_hash() const
{
	return mHash;
}

bool subtexture::load(tinyxml2::XMLElement * pEle)
{
	assert(pEle != nullptr);

	const char* name;
	if (pEle->QueryStringAttribute("name", &name) != tinyxml2::XML_SUCCESS)
		name = pEle->Name(); // for backwards compatibility
	set_name(name);

	// Set root frame
	frect frame = {
		pEle->FloatAttribute("x"),
		pEle->FloatAttribute("y"),
		pEle->FloatAttribute("w"),
		pEle->FloatAttribute("h")
	};
	set_frame_rect(frame);
	set_frame_count(pEle->IntAttribute("frames", 1));
	add_interval(0, pEle->FloatAttribute("interval", 0));
	set_default_frame(pEle->IntAttribute("default", 0));

	// Set loop type (default : none)
	bool att_loop = pEle->BoolAttribute("loop");
	bool att_pingpong = pEle->BoolAttribute("pingpong");
	engine::animation::loop_type loop_type = engine::animation::loop_type::none;
	if (att_loop)                loop_type = engine::animation::loop_type::linear;
	if (att_pingpong)            loop_type = engine::animation::loop_type::pingpong;
	set_loop(loop_type);

	// Setup sequence for changing of interval over time
	auto ele_seq = pEle->FirstChildElement("seq");
	while (ele_seq)
	{
		add_interval(
			(engine::frame_t)ele_seq->IntAttribute("from"),
			ele_seq->FloatAttribute("interval"));
		ele_seq = ele_seq->NextSiblingElement();
	}
	return true;
}

bool subtexture::save(tinyxml2::XMLElement * pEle)
{
	assert(pEle != nullptr);

	pEle->SetAttribute("name", mName.c_str());

	auto root_frame = get_root_frame();
	pEle->SetAttribute("x", root_frame.x);
	pEle->SetAttribute("y", root_frame.y);
	pEle->SetAttribute("w", root_frame.w);
	pEle->SetAttribute("h", root_frame.h);

	if (get_frame_count() > 1)
		pEle->SetAttribute("frames", static_cast<unsigned int>(get_frame_count()));

	if (get_interval() > 0)
		pEle->SetAttribute("interval", get_interval());

	if (get_default_frame() != 0)
		pEle->SetAttribute("default", static_cast<unsigned int>(get_default_frame()));

	switch (get_loop())
	{
	case animation::loop_type::linear:
		pEle->SetAttribute("loop", 1);
		break;
	case animation::loop_type::pingpong:
		pEle->SetAttribute("pingpong", 1);
		break;
	default: break;
	}
	
	// TODO: Save sequenced interval
	return true;
}

bool texture_atlas::load(const std::string & pPath)
{
	clear();

	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
		return false;
	return load_entries(doc);
}

bool texture_atlas::save(const std::string & pPath) const
{
	using namespace tinyxml2;

	XMLDocument doc;
	auto root = doc.NewElement("atlas");
	doc.InsertEndChild(root);
	
	for (auto& i : mAtlas)
	{
		auto entry = doc.NewElement("subtexture");
		i->save(entry);
		root->InsertEndChild(entry);
	}
	return doc.SaveFile(pPath.c_str()) == XML_SUCCESS;
}

bool texture_atlas::load_memory(const char * pData, size_t pSize)
{
	clear();

	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.Parse(pData, pSize))
		return false;
	return load_entries(doc);
}

void texture_atlas::clear()
{
	mAtlas.clear();
}
std::shared_ptr<subtexture> texture_atlas::get_entry(const std::string & pName) const
{
	hash::hash32_t cmphash = hash::hash32(pName);
	for (auto& i : mAtlas)
		if (i->get_hash() == cmphash)
			return i;
	return {};
}

std::shared_ptr<subtexture> texture_atlas::get_entry(const fvector & pVec) const
{
	for (auto& i : mAtlas)
		if (i->get_root_frame().is_intersect(pVec))
			return i;
	return{};
}

bool texture_atlas::add_entry(const subtexture & pEntry)
{
	if (get_entry(pEntry.get_name()))
		return false;
	mAtlas.push_back(std::make_shared<subtexture>(pEntry));
	return true;
}

bool texture_atlas::add_entry(subtexture::ptr & pEntry)
{
	if (get_entry(pEntry->get_name()))
		return false;
	mAtlas.push_back(pEntry);
	return true;
}

bool texture_atlas::rename_entry(const std::string & pOriginal, const std::string & pRename)
{
	if (pOriginal == pRename)
		return false;

	auto entry = get_entry(pOriginal);
	if (!entry)
		return false;

	entry->set_name(pRename);

	return true;
}

bool texture_atlas::remove_entry(const std::string & pName)
{
	for (size_t i = 0; i < mAtlas.size(); i++)
		if (mAtlas[i]->get_name() == pName)
		{
			mAtlas.erase(mAtlas.begin() + i);
			return true;
		}
	return false;
}

bool engine::texture_atlas::remove_entry(subtexture::ptr & pEntry)
{
	for (size_t i = 0; i < mAtlas.size(); i++)
		if (mAtlas[i] == pEntry)
		{
			mAtlas.erase(mAtlas.begin() + i);
			return true;
		}
	return false;
}

std::vector<std::string> texture_atlas::compile_list() const
{
	std::vector<std::string> list;
	for (auto& i : mAtlas)
		list.push_back(i->get_name());
	return std::move(list);
}

const std::vector<subtexture::ptr>& texture_atlas::get_all() const
{
	return mAtlas;
}

bool texture_atlas::empty() const
{
	return mAtlas.empty();
}


bool texture_atlas::load_entries(tinyxml2::XMLDocument& pDoc)
{
	auto ele_atlas = pDoc.FirstChildElement("atlas");
	if (!ele_atlas)
		return false;

	auto ele_entry = ele_atlas->FirstChildElement();
	while (ele_entry)
	{
		// TODO: Check for colliding names
		subtexture entry;
		entry.load(ele_entry);
		add_entry(entry);

		ele_entry = ele_entry->NextSiblingElement();
	}
	return true;
}

void texture::set_texture_source(const std::string& pFilepath)
{
	mTexture_source = pFilepath;
}

std::string texture::get_texture_source()
{
	return mTexture_source;
}

void texture::set_atlas_source(const std::string & pFilepath)
{
	mAtlas_source = pFilepath;
}

std::string texture::get_atlas_source()
{
	return mAtlas_source;
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
				if (!mAtlas.load(mAtlas_source))
					logger::error("Failed to load atlas '" + mAtlas_source + "'");
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

std::shared_ptr<subtexture> texture::get_entry(const std::string & pName) const
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

texture_atlas & engine::texture::get_texture_atlas()
{
	return mAtlas;
}
