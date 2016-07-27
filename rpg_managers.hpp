#ifndef RPG_MANAGERS_HPP
#define RPG_MANAGERS_HPP

#include <string>
#include <vector>
#include <list>
#include "texture.hpp"

namespace rpg
{

class texture_manager
{
	struct texture_entry
	{
		std::string name, path, atlas;
		bool is_loaded, has_atlas;
		engine::texture tex;
	};
	std::list<texture_entry> textures;
	texture_entry* find_entry(std::string name);

public:
	int load_settings(std::string path);
	engine::texture* get_texture(std::string name);
	std::vector<std::string> construct_list();
};

}

#endif // !RPG_MANAGERS_HPP
