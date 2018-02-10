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
#include <set>

namespace engine
{

class subtexture :
	public animation
{
public:
	subtexture() {}
	subtexture(const std::string& pName);

	typedef std::shared_ptr<subtexture> ptr;
	
	void set_name(const std::string& pName);
	const std::string& get_name() const;

	bool load(tinyxml2::XMLElement* pEle);
	bool save(tinyxml2::XMLElement* pEle);

private:
	std::string mName;
};

class texture_atlas
{
public:
	bool load(const std::string& pPath);
	bool save(const std::string& pPath);
	bool load_memory(const char* pData, size_t pSize);
	void clear();

	subtexture::ptr get_entry(const std::string& pName) const;
	subtexture::ptr get_entry(const fvector& pVec) const;

	bool add_entry(const subtexture& pEntry);
	bool add_entry(subtexture::ptr& pEntry);

	bool rename_entry(const std::string& pOriginal, const std::string& pRename);

	bool remove_entry(const std::string& pName);
	bool remove_entry(subtexture::ptr& pEntry);

	std::vector<std::string> compile_list() const;

	const std::vector<subtexture::ptr>& get_raw_atlas() const;

	bool is_empty() const;

private:
	bool load_entries(tinyxml2::XMLDocument& pDoc);

	std::vector<subtexture::ptr> mAtlas;
};

class texture :
	public resource
{
public:
	void set_texture_source(const std::string& pFilepath);
	void set_atlas_source(const std::string& pFilepath);
	bool load();
	bool unload();

	std::shared_ptr<subtexture> get_entry(const std::string& pName) const;

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