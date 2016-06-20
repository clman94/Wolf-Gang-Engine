#ifndef ENGINE_TEXTURE_HPP
#define ENGINE_TEXTURE_HPP

#include <vector>
#include <string>
#include <SFML\Graphics.hpp>

namespace engine
{	
struct texture_crop
{
	int x, y, w, h;
	std::string name;
	unsigned int id; // optionally for those without names
};
class texture
{
	std::vector<texture_crop> atlas;
	sf::Texture _texture;
public:

	int load_texture(const std::string path);
	int load_atlas(const std::string path);
#ifdef ENGINE_INTERNAL
	int find_atlas(std::string name, texture_crop& crop);
	int find_atlas(int id, texture_crop& crop);
	int get_atlas_count()
	{
		return atlas.size();
	}
	texture_crop get_crop(int index)
	{
		return atlas[index];
	}

	sf::Texture& sfml_get_texture()
	{
		return _texture;
	}
#endif
};

}

#endif