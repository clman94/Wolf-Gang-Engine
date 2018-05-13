#define ENGINE_INTERNAL

#include <engine/texture.hpp>
#include <engine/logger.hpp>

#include "../../3rdparty/tinyxml2/tinyxml2.h"

#include <iostream>

#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#include <stb_rect_pack.h> // This is included with imgui. So, why not use it, right?

using namespace engine;

subtexture::subtexture(const std::string & pName)
{
	set_name(pName);
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

texture_atlas::texture_atlas(const std::vector<subtexture>& pArray)
{
	for (auto& i : pArray)
		mAtlas.push_back(std::make_shared<subtexture>(i));
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

bool texture_atlas::remove_entry(subtexture::ptr & pEntry)
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

texture::texture()
{
	mIs_using_cache = false;
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

bool texture::generate_texture(const fs::path& pCache_path) const
{
	logger::info("generate_texture: Loading texture...");

	sf::Image image;
	if (!image.loadFromFile(mTexture_source))
	{
		logger::error("Cannot open image");
		return false;
	}

	texture_atlas atlas;
	if (!atlas.load(mAtlas_source))
	{
		logger::error("Cannot open atlas");
		return false;
	}

	logger::info("generate_texture: Calculating new atlas...");

	// Setup all the rects for the stb rect packer to use
	std::vector<stbrp_rect> rects(atlas.get_all().size());
	for (size_t i = 0; i < rects.size(); i++)
	{
		rects[i].id = static_cast<int>(i); 

		engine::frect full_rect = atlas.get_all()[i]->get_full_rect();
		rects[i].w = static_cast<stbrp_coord>(full_rect.w + 2); // "+ 2" for the padding
		rects[i].h = static_cast<stbrp_coord>(full_rect.h + 2);
	}

	// Use the stb rect packer to generate the new positions for the atlas entries
	stbrp_context ctx;
	const int node_count = 1024;
	std::vector<stbrp_node> nodes(node_count);
	stbrp_init_target(&ctx, 1024, 1024, &nodes[0], node_count);
	if (stbrp_pack_rects(&ctx, &rects[0], rects.size()) == 0)
	{
		logger::error("Unable to pack all entries.");
		return false;
	}

	// Create a new atlas for the repositioned subtextures
	std::vector<subtexture> new_atlas_array(atlas.get_all().size());
	engine::fvector new_size; // This will be the new size for the texture
	for (const stbrp_rect& i : rects)
	{
		const engine::frect packed_rect(i.x + 1, i.y + 1, i.w - 2, i.h - 2); // New rect excluding the padding

		// Update the size to fit this entry
		new_size.x = std::max(new_size.x, packed_rect.x + packed_rect.w);
		new_size.y = std::max(new_size.y, packed_rect.y + packed_rect.h);

		// Copy the original and update the position
		subtexture::ptr orig_atlas = atlas.get_all()[i.id];
		new_atlas_array[i.id] = *orig_atlas;
		new_atlas_array[i.id].set_frame_rect({ packed_rect.get_offset(), orig_atlas->get_root_frame().get_size() });
	}
	new_size.ceil();
	const texture_atlas new_atlas(new_atlas_array);

	logger::info("generate_texture: Copying to new texture...");

	// Copy the image to the new image
	sf::Image new_image;
	new_image.create(new_size.x, new_size.y, sf::Color(0, 0, 0, 0));
	for (size_t i = 0; i < atlas.get_all().size(); i++)
	{
		const engine::frect new_rect = new_atlas.get_all()[i]->get_full_rect();
		const engine::frect old_rect = atlas.get_all()[i]->get_full_rect();

		// Duplicate the perimiter of the image into the padding of the entries
		// This effectively hides any floating point errors
		new_image.copy(image, new_rect.x - 1, new_rect.y
			, engine::rect_cast<int>(engine::frect(old_rect.get_offset(), { 1, old_rect.get_size().y})));
		new_image.copy(image, new_rect.x, new_rect.y - 1
			, engine::rect_cast<int>(engine::frect(old_rect.get_offset(), { old_rect.get_size().x, 1 })));
		new_image.copy(image, new_rect.x + new_rect.w, new_rect.y
			, engine::rect_cast<int>(engine::frect(old_rect.get_offset() + engine::fvector(old_rect.w - 1, 0), { 1, old_rect.get_size().y })));
		new_image.copy(image, new_rect.x, new_rect.y + new_rect.h
			, engine::rect_cast<int>(engine::frect(old_rect.get_offset() + engine::fvector(0, old_rect.h - 1), { old_rect.get_size().x, 1 })));

		// Copy the main image
		new_image.copy(image, new_rect.x, new_rect.y, engine::rect_cast<int>(old_rect));
	}

	logger::info("generate_texture: Saving to cache...");

	// Make sure the directory exists
	fs::path textures_cache = pCache_path / "textures";
	fs::create_directories(textures_cache);

	// Save everything!
	new_image.saveToFile((textures_cache / (get_name() + ".png")).string());
	new_atlas.save((textures_cache / (get_name() + ".xml")).string());

	logger::info("generate_texture: Generating margined texture complete.");

	return true;
}

bool texture::load()
{
	if (!is_loaded())
	{
		mSFML_texture.reset(new sf::Texture());

		std::string final_texture_source = mTexture_source;
		std::string final_atlas_source = mAtlas_source;
		if (!mCache_path.empty() && check_for_cache())
		{
			mIs_using_cache = true;
			final_texture_source = get_cached_texture_path().string();
			final_atlas_source = get_cached_atlas_path().string();
		}

		if (mPack)
		{
			{ // Texture
				auto data = mPack->read_all(final_texture_source);
				set_loaded(mSFML_texture->loadFromMemory(&data[0], data.size()));
			}
			if (!mAtlas_source.empty())
			{ // Atlas
				auto data = mPack->read_all(final_atlas_source);
				mAtlas.load_memory(&data[0], data.size());
			}
		}
		else
		{
			set_loaded(mSFML_texture->loadFromFile(final_texture_source));
			if (!mAtlas_source.empty())
			{
				if (!mAtlas.load(final_atlas_source))
					logger::error("Failed to load atlas '" + final_atlas_source + "'");
			}
			else
			{
				// Create a default if the atlas does not exist
				mAtlas.clear();
				subtexture deftex;
				deftex.set_name("default:default");
				deftex.set_frame_rect({ 0, 0, get_size().x, get_size().y });
				mAtlas.add_entry(deftex);
			}
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

sf::Texture & texture::get_sfml_texture()
{
	load(); // Ensure load
	return *mSFML_texture;
}

void texture::set_cache_directory(const fs::path & pPath)
{
	mCache_path = pPath;
}

bool texture::is_using_cache() const
{
	return mIs_using_cache;
}

fs::path texture::get_cached_texture_path() const
{
	return mCache_path / fs::path(mTexture_source).filename();
}

fs::path texture::get_cached_atlas_path() const
{
	return mCache_path / fs::path(mAtlas_source).filename();
}

bool texture::check_for_cache() const
{
	return fs::exists(get_cached_texture_path())
		&& fs::exists(get_cached_texture_path());
}
