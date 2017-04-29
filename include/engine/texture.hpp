#ifndef ENGINE_TEXTURE_HPP
#define ENGINE_TEXTURE_HPP

#include <SFML/Graphics.hpp>

#include <engine/animation.hpp>
#include <engine/rect.hpp>
#include <engine/types.hpp>
#include <engine/resource.hpp>
#include <engine/utility.hpp>

#include "../../tinyxml2/tinyxml2.h"

#include <map>
#include <vector>
#include <string>
#include <assert.h>

namespace engine
{

class atlas_entry
{
public:
	atlas_entry();
	atlas_entry(std::shared_ptr<engine::animation> pAnimation);
	frect get_root_rect() const;
	bool is_animation() const;
	std::shared_ptr<animation> get_animation() const;

	bool load(tinyxml2::XMLElement* pEle);
	bool save(tinyxml2::XMLElement* pEle);

private:
	std::shared_ptr<animation> mAnimation;
};

class texture_atlas
{
public:
	bool load(const std::string& pPath);
	bool save(const std::string& pPath);
	bool load_memory(const char* pData, size_t pSize);
	void clean();

	util::optional_pointer<const atlas_entry> get_entry(const std::string& pName) const;
	util::optional_pointer<const std::pair<const std::string, atlas_entry>> get_entry(const fvector& pVec) const;

	bool add_entry(const std::string& pName, const atlas_entry& pEntry);
	bool rename_entry(const std::string& pOriginal, const std::string& pRename);
	bool remove_entry(const std::string& pName);

	std::vector<std::string> compile_list() const;

	const std::map<std::string, atlas_entry>& get_raw_atlas() const;

private:
	bool load_settings(tinyxml2::XMLDocument& pDoc);
	std::map<std::string, atlas_entry> mAtlas;
};

class texture :
	public resource
{
public:
	void set_texture_source(const std::string& pFilepath);
	void set_atlas_source(const std::string& pFilepath);
	bool load();
	bool unload();

	util::optional_pointer<const atlas_entry> get_entry(const std::string& pName) const;

	std::vector<std::string> compile_list() const;

	fvector get_size() const;

#ifdef ENGINE_INTERNAL
	sf::Texture& sfml_get_texture()
	{
		load(); // Ensure load
		return *mSFML_texture;
	}
#endif

private:
	std::string mTexture_source;
	std::string mAtlas_source;
	texture_atlas mAtlas;
	std::unique_ptr<sf::Texture> mSFML_texture;
};



}

#endif