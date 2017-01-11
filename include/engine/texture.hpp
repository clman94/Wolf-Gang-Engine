#ifndef ENGINE_TEXTURE_HPP
#define ENGINE_TEXTURE_HPP

#include <SFML/Graphics.hpp>

#include <engine/animation.hpp>
#include <engine/rect.hpp>
#include <engine/types.hpp>
#include <engine/resource.hpp>
#include <engine/utility.hpp>

#include "../../tinyxml2/tinyxml2.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <assert.h>

namespace engine
{

class atlas_entry
{
public:
	frect get_root_rect() const;
	bool is_animation() const;
	const engine::animation& get_animation() const;

	bool load(tinyxml2::XMLElement* pEle);
private:
	bool mIs_animation;
	engine::animation mAnimation;
};

class texture_atlas
{
public:
	bool load(const std::string& pPath);

	void clean();

	util::optional_pointer<const atlas_entry> get_entry(const std::string& pName) const;

	std::vector<std::string> compile_list() const;

private:
	std::unordered_map<std::string, atlas_entry> mAtlas;
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