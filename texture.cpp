#define ENGINE_INTERNAL

#include "texture.hpp"
#include "tinyxml2\tinyxml2.h"
#include <iostream>

using namespace engine;

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
	auto &find = mAtlas.find(pName);
	if (find != mAtlas.end())
		return find->second.rect;
	return frect();
}

const animation* texture::get_animation(const std::string & pName)
{
	auto &find = mAtlas.find(pName);
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
	engine::animation::e_loop loop_type = engine::animation::e_loop::none;
	if (att_loop)             loop_type = engine::animation::e_loop::linear;
	if (att_pingpong)         loop_type = engine::animation::e_loop::pingpong;
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
}
