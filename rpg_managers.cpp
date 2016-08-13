#include "rpg.hpp"
#include "rpg_managers.hpp"

using namespace rpg;

texture_manager::texture_entry*
texture_manager::find_entry(const std::string&  pName)
{
	for (auto& i : mTextures)
	{
		if (i.name == pName)
			return &i;
	}
	return nullptr;
}

int
texture_manager::load_settings(tinyxml2::XMLElement* pEle)
{
	assert(pEle != nullptr);

	using namespace tinyxml2;

	XMLElement* ele_entry = pEle->FirstChildElement();
	while (ele_entry)
	{
		texture_entry nentry;
		nentry.name = ele_entry->Name();
		nentry.path = ele_entry->Attribute("path");

		// the optional atlas path
		const char* att_atlas = ele_entry->Attribute("atlas");
		if (att_atlas)
		{
			nentry.atlas = att_atlas;
			nentry.has_atlas = true;
		}
		else
			nentry.has_atlas = false;

		nentry.is_loaded = false;
		mTextures.push_back(nentry);
		ele_entry = ele_entry->NextSiblingElement();
	}
	return 0;
}

engine::texture*
texture_manager::get_texture(const std::string&  pName)
{
	texture_entry* entry = find_entry(pName);
	if (!entry)
	{
		std::cout << "Error: Texture with name '" << pName << "' Does not exist.\n";
		return nullptr;
	}
	if (!entry->is_loaded)
	{
		entry->tex.load_texture(entry->path);
		if (entry->has_atlas)
			entry->tex.load_atlas_xml(entry->atlas);
		entry->is_loaded = true;
	}
	return &entry->tex;
}

std::vector<std::string>
texture_manager::construct_list()
{
	std::vector<std::string> retval;
	retval.reserve(mTextures.size());
	for (auto& i : mTextures)
		retval.push_back(i.name);
	return std::move(retval);
}

sound_manager::soundbuffer_entry*
sound_manager::find_buffer(const std::string&  pName)
{
	for (auto& i : mBuffers)
		if (i.name == pName)
			return &i;
	return nullptr;
}

util::error
sound_manager::load_sounds(const std::string&  pPath)
{
	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(pPath.c_str()))
		return "Could not load souind manager";

	XMLElement* ele_root = doc.RootElement();
	if (!ele_root)
		return "Please add root node";

	auto ele_sound = ele_root->FirstChildElement();
	while (ele_sound)
	{
		mBuffers.emplace_back();
		auto & nbuf = mBuffers.back();

		nbuf.buffer.load(pPath);
		nbuf.name = util::safe_string(ele_sound->Name());

		ele_sound->NextSiblingElement();
	}

	return 0;
}

int
sound_manager::spawn_sound(const std::string& name)
{
	auto buf = find_buffer(name);
	if (!buf)
		return 1;
	mSounds.spawn(buf->buffer);
	return 0;
}
