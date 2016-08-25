#ifndef ENGINE_TEXTURE_HPP
#define ENGINE_TEXTURE_HPP

#include <vector>
#include <string>
#include <assert.h>
#include <SFML\Graphics.hpp>
#include "animation.hpp"
#include "rect.hpp"
#include "types.hpp"
#include "tinyxml2\tinyxml2.h"
#include <unordered_map>

namespace engine
{

class texture
{
public:
	int load_texture(const std::string& pPath);
	void add_entry(const std::string& pName, const frect pRect);
	frect get_entry(const std::string& pName);
	const engine::animation* get_animation(const std::string& pName);
	int load_atlas_xml(const std::string& pPath);

#ifdef ENGINE_INTERNAL
	sf::Texture& sfml_get_texture()
	{ return mTexture; }
#endif

private:
	struct entry
	{
		frect rect;
		bool is_animation;
		engine::animation animation;
		bool load_rect_xml(tinyxml2::XMLElement* pEle);
	};
	bool load_animation_xml(tinyxml2::XMLElement* pEle, entry& entry);
	std::unordered_map<std::string, entry> mAtlas;
	sf::Texture mTexture;
};



}

#endif