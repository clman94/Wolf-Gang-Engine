#ifndef RPG_MANAGERS_HPP
#define RPG_MANAGERS_HPP

#include <string>
#include <vector>
#include <list>
#include "texture.hpp"
#include "audio.hpp"
#include "tinyxml2\xmlshortcuts.hpp"

namespace rpg
{

class texture_manager
{
public:
	int load_settings(tinyxml2::XMLElement* pEle);
	engine::texture* get_texture(const std::string& pName);
	std::vector<std::string> construct_list();

public:
	struct texture_entry
	{
		std::string name, path, atlas;
		bool is_loaded, has_atlas;
		engine::texture tex;
	};
	std::list<texture_entry> mTextures;
	texture_entry* find_entry(const std::string& pName);
};

class sound_manager
{
public:
	util::error load_sounds(const std::string& pPath);
	int spawn_sound(const std::string& pName);

private:
	struct soundbuffer_entry
	{
		std::string name, path;
		engine::sound_buffer buffer;
	};
	std::list<soundbuffer_entry> mBuffers;
	soundbuffer_entry* find_buffer(const std::string& pName);

	engine::sound_spawner mSounds;
};

}

#endif // !RPG_MANAGERS_HPP
