#ifndef ENGINE_TEXTURE_HPP
#define ENGINE_TEXTURE_HPP

#include <vector>
#include <string>
#include <SFML\Graphics.hpp>
#include "rect.hpp"
#include "types.hpp"

namespace engine
{	

struct texture_crop :
	public irect
{
	std::string name;
};

class texture
{
	std::vector<texture_crop> atlas;
	sf::Texture _texture;
public:

	int load_texture(const std::string path);
	void add_entry(const std::string name, const engine::irect rect);
	engine::irect get_entry(const std::string name);
#ifdef ENGINE_INTERNAL
	sf::Texture& sfml_get_texture()
	{
		return _texture;
	}
#endif
};

int load_xml_atlas(texture& tex, const std::string path);

}

#endif