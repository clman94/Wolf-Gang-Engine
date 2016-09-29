#ifndef RPG_MANAGERS_HPP
#define RPG_MANAGERS_HPP

#include <engine/texture.hpp>
#include <engine/audio.hpp>
#include "../../tinyxml2/xmlshortcuts.hpp"

#include <string>
#include <vector>
#include <list>
#include <unordered_map>

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
		std::string path, atlas;
		bool is_loaded, has_atlas;
		engine::texture texture;
		bool ensure_loaded();
		
	};
	std::map<std::string, texture_entry> mTextures;
};

class sound_manager
{
public:
	util::error load_sounds(tinyxml2::XMLElement* pEle_root);
	int spawn_sound(const std::string& pName);
	void stop_all();

private:
	std::unordered_map<std::string, engine::sound_buffer> mBuffers;
	engine::sound_spawner mSounds;
};

}

#endif // !RPG_MANAGERS_HPP
