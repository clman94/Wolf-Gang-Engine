#ifndef ENGINE_TEXTURE_HPP
#define ENGINE_TEXTURE_HPP

#include <SFML/Graphics.hpp>

#include <engine/animation.hpp>
#include <engine/rect.hpp>
#include <engine/resource.hpp>
#include <engine/utility.hpp>
#include <engine/hash.hpp>

#include "../../3rdparty/tinyxml2/tinyxml2.h"

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

	hash::hash32_t get_hash() const; // Helps speed up searching

	bool load(tinyxml2::XMLElement* pEle);
	bool save(tinyxml2::XMLElement* pEle);

private:
	std::string mName;
	hash::hash32_t mHash;
};

class texture_atlas
{
public:
	bool load(const std::string& pPath);
	bool load_memory(const char* pData, size_t pSize);
	bool save(const std::string& pPath) const;

	void clear();

	subtexture::ptr get_entry(const std::string& pName) const;
	subtexture::ptr get_entry(const fvector& pVec) const;

	bool add_entry(const subtexture& pEntry);
	bool add_entry(subtexture::ptr& pEntry);

	bool rename_entry(const std::string& pOriginal, const std::string& pRename);
	
	bool remove_entry(const std::string& pName);
	bool remove_entry(subtexture::ptr& pEntry);

	std::vector<std::string> compile_list() const;

	const std::vector<subtexture::ptr>& get_all() const;

	bool is_empty() const;

private:
	bool load_entries(tinyxml2::XMLDocument& pDoc);

	std::vector<subtexture::ptr> mAtlas;
};

const std::string texture_restype = "texture";

class texture :
	public resource
{
public:

	void set_texture_source(const std::string& pFilepath);
	void set_atlas_source(const std::string& pFilepath);

	bool load() override;
	bool unload() override;

	const std::string& get_type() const override
	{
		return texture_restype;
	}

	std::shared_ptr<subtexture> get_entry(const std::string& pName) const;

	std::vector<std::string> compile_list() const;

	fvector get_size() const;

	texture_atlas& get_texture_atlas();

	sf::Texture& get_sfml_texture()
	{
		load(); // Ensure load
		return *mSFML_texture;
	}

private:
	std::string mTexture_source;
	std::string mAtlas_source;
	texture_atlas mAtlas;
	std::unique_ptr<sf::Texture> mSFML_texture;
};

}

#endif