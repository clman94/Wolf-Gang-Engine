#ifndef RPG_MANAGERS_HPP
#define RPG_MANAGERS_HPP

#include <string>
#include <vector>
#include <list>
#include "texture.hpp"
#include "audio.hpp"

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

class sound_manager
{
	struct soundbuffer_entry
	{
		std::string name, path;
		engine::sound_buffer buffer;
	};
	std::list<soundbuffer_entry> buffers;
	soundbuffer_entry* find_buffer(std::string name);

	engine::sound_spawner sounds;

public:
	util::error load_sounds(std::string path);
	int spawn_sound(const std::string& name);
};

}

#endif // !RPG_MANAGERS_HPP
