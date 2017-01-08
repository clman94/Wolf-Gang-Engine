#ifndef RPG_MANAGERS_HPP
#define RPG_MANAGERS_HPP

#include <engine/texture.hpp>
#include <engine/resource.hpp>
#include <engine/audio.hpp>
#include <engine/utility.hpp>
#include "../../tinyxml2/xmlshortcuts.hpp"

#include <string>
#include <vector>
#include <list>
#include <unordered_map>

namespace rpg
{

bool load_texture_resources(const std::string& pDirectory, engine::resource_manager& pResource_manager);
bool load_sound_resources(const std::string& pDirectory, engine::resource_manager& pResource_manager);

}

#endif // !RPG_MANAGERS_HPP
