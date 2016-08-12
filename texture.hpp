#ifndef ENGINE_TEXTURE_HPP
#define ENGINE_TEXTURE_HPP

#include <vector>
#include <string>
#include <SFML\Graphics.hpp>
#include "rect.hpp"
#include "types.hpp"
#include <unordered_map>

namespace engine
{

class texture
{
public:
	int load_texture(const std::string& path);
	void add_entry(const std::string& name, const irect rect);
	irect get_entry(const std::string& name);
	int load_atlas_xml(const std::string& path);

#ifdef ENGINE_INTERNAL
	sf::Texture& sfml_get_texture()
	{ return _texture; }
#endif

private:
	std::unordered_map<std::string, irect> atlas;
	sf::Texture _texture;
};



}

#endif