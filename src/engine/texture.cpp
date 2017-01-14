#define ENGINE_INTERNAL

#include <engine/texture.hpp>

#include "../../tinyxml2/tinyxml2.h"

#include <iostream>

using namespace engine;

atlas_entry::atlas_entry()
{
	mAnimation = std::make_shared<animation>();
}

frect atlas_entry::get_root_rect() const
{
	return mAnimation->get_frame_at(0);
}

bool atlas_entry::is_animation() const
{
	return false;
}

std::shared_ptr<const engine::animation> atlas_entry::get_animation() const
{
	return mAnimation;
}

bool atlas_entry::load(tinyxml2::XMLElement * pEle)
{
	assert(pEle != nullptr);

	// Set root frame

	frect rect;
	rect.x = pEle->FloatAttribute("x");
	rect.y = pEle->FloatAttribute("y");
	rect.w = pEle->FloatAttribute("w");
	rect.h = pEle->FloatAttribute("h");
	mAnimation->set_frame_rect(rect);

	int att_frames = pEle->IntAttribute("frames");
	float att_interval = pEle->FloatAttribute("interval");

	// "frames" and "interval" are required for animation both with a value > 0
	if (att_frames <= 0 || att_interval <= 0)
	{
		mIs_animation = false;
		return true;
	}

	// Set frame count

	engine::frame_t frame_count = (att_frames <= 0 ? 1 : att_frames);// Default one frame
	mAnimation->set_frame_count(frame_count);

	// Set starting interval

	mAnimation->add_interval(0, att_interval);

	// Set default frame (default : 0)

	int att_default = pEle->IntAttribute("default");
	mAnimation->set_default_frame(att_default);

	// Set loop type (default : none)

	bool att_loop = pEle->BoolAttribute("loop");
	bool att_pingpong = pEle->BoolAttribute("pingpong");

	engine::animation::loop_type loop_type = engine::animation::loop_type::none;
	if (att_loop)             loop_type = engine::animation::loop_type::linear;
	if (att_pingpong)         loop_type = engine::animation::loop_type::pingpong;
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

	mIs_animation = true;
	return true;
}

bool texture_atlas::load(const std::string & pPath)
{
	clean();

	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
		return false;

	auto ele_atlas = doc.FirstChildElement("atlas");
	if (!ele_atlas)
		return false;

	auto ele_entry = ele_atlas->FirstChildElement();
	while (ele_entry)
	{
		// TODO: Check for colliding names
		mAtlas[ele_entry->Name()].load(ele_entry);

		ele_entry = ele_entry->NextSiblingElement();
	}
	return false;
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

std::vector<std::string> texture_atlas::compile_list() const
{
	std::vector<std::string> list;
	for (auto& i : mAtlas)
		list.push_back(i.first);
	return std::move(list);
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
		set_loaded(mSFML_texture->loadFromFile(mTexture_source));
		mAtlas.load(mAtlas_source);
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




/*
int
texture::load_texture(const std::string& pPath)
{
	if (!mTexture.loadFromFile(pPath))
	{
		return 1;
	}
	return 0;
}

void
texture::add_entry(const std::string& pName, const frect pRect)
{
	mAtlas[pName].rect = pRect;
}

frect
texture::get_entry(const std::string& pName)
{
	auto find = mAtlas.find(pName);
	if (find != mAtlas.end())
		return find->second.rect;
	return frect();
}

const animation* texture::get_animation(const std::string & pName)
{
	auto find = mAtlas.find(pName);
	if (find != mAtlas.end())
		return &find->second.animation;
	return nullptr;
}

int
texture::load_atlas_xml(const std::string& pPath)
{
	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
		return 1;

	auto atlas_e = doc.FirstChildElement("atlas");
	if (!atlas_e)
		return 2;

	// Iterate though each item and load their values into the mAtlas
	auto ele_crop = atlas_e->FirstChildElement();
	while (ele_crop)
	{
		auto &nentry = mAtlas[ele_crop->Name()];
		nentry.load_rect_xml(ele_crop);
		nentry.is_animation = load_animation_xml(ele_crop, nentry);
		nentry.animation.set_texture(*this);
		ele_crop = ele_crop->NextSiblingElement();
	}
	return 0;
}

std::vector<std::string> engine::texture::compile_list()
{
	std::vector<std::string> list;
	for (auto& i : mAtlas)
		list.push_back(i.first);
	return std::move(list);
}

bool texture::load_animation_xml(tinyxml2::XMLElement * pEle, entry& pEntry)
{
	assert(pEle != nullptr);

	int att_frames = pEle->IntAttribute("frames");
	if (att_frames == 0) // "frames" is required for animation
		return false;

	auto &anim = pEntry.animation;

	int att_interval = pEle->IntAttribute("interval");
	if (att_interval > 0)
		anim.add_interval(0, att_interval);

	int att_default = pEle->IntAttribute("default");
	anim.set_default_frame(att_default);

	bool att_loop = pEle->BoolAttribute("loop");
	bool att_pingpong = pEle->BoolAttribute("pingpong");
	engine::animation::loop_type loop_type = engine::animation::loop_type::none;
	if (att_loop)             loop_type = engine::animation::loop_type::linear;
	if (att_pingpong)         loop_type = engine::animation::loop_type::pingpong;
	anim.set_loop(loop_type);

	engine::frame_t frame_count = (att_frames <= 0 ? 1 : att_frames);// Default one frame
	anim.set_frame_count(frame_count);

	auto ele_seq = pEle->FirstChildElement("seq");
	while (ele_seq)
	{
		anim.add_interval(
			(engine::frame_t)ele_seq->IntAttribute("from"),
			ele_seq->IntAttribute("interval"));
		ele_seq = ele_seq->NextSiblingElement();
	}
	return true;
}

bool texture::entry::load_rect_xml(tinyxml2::XMLElement * pEle)
{
	rect.x = pEle->FloatAttribute("x");
	rect.y = pEle->FloatAttribute("y");
	rect.w = pEle->FloatAttribute("w");
	rect.h = pEle->FloatAttribute("h");
	animation.set_frame_rect(rect);
	return true;
}*/

