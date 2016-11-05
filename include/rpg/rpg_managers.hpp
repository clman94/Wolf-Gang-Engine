#ifndef RPG_MANAGERS_HPP
#define RPG_MANAGERS_HPP

#include <engine/texture.hpp>
#include <engine/audio.hpp>
#include <engine/utility.hpp>
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

	// Locates all png files in the directory and associates its
	// atlas of the same name (if it exists)
	int load_from_directory(const std::string& pPath);
	util::optional_pointer<engine::texture> get_texture(const std::string& pName);
	std::vector<std::string> construct_list();

public:
	struct texture_entry
	{
		std::string path, atlas;
		bool is_loaded;
		engine::texture texture;
		bool ensure_loaded();
	};
	std::map<std::string, texture_entry> mTextures;
};

class sound_manager
{
public:
	int load_sounds(tinyxml2::XMLElement* pEle_root);
	bool spawn_sound(const std::string& pName, float pVolume = 100, float pPitch = 1);
	void stop_all();

private:
	std::unordered_map<std::string, engine::sound_buffer> mBuffers;
	engine::sound_spawner mSounds;
};

}

#endif // !RPG_MANAGERS_HPP
