#ifndef RPG_GAME_SETTINGS_LOADER_HPP
#define RPG_GAME_SETTINGS_LOADER_HPP

#include <string>
#include <engine/vector.hpp>
#include <engine/controls.hpp>

#include "../../tinyxml2/tinyxml2.h"

namespace rpg {

class game_settings_loader
{
public:
	bool load(const std::string& pPath, const std::string& pPrefix_path = std::string());
	bool load_memory(const char* pData, size_t pSize, const std::string& pPrefix_path = std::string());

	const std::string& get_start_scene() const;
	const std::string& get_textures_path() const;
	const std::string& get_sounds_path() const;
	const std::string& get_music_path() const;
	const std::string& get_fonts_path() const;
	const std::string& get_scenes_path() const;
	const std::string& get_player_texture() const;
	engine::fvector    get_screen_size() const;
	float get_unit_pixels() const;
	const engine::controls& get_key_bindings() const;

private:
	bool parse_settings(tinyxml2::XMLDocument& pDoc, const std::string& pPrefix_path);
	bool parse_key_bindings(tinyxml2::XMLElement* pEle);
	bool parse_binding_attributes(tinyxml2::XMLElement* pEle, const std::string& pName
		, const std::string& pPrefix, bool pAlternative);

	std::string mStart_scene;
	std::string mTextures_path;
	std::string mSounds_path;
	std::string mMusic_path;
	std::string mPlayer_texture;
	std::string mFonts_path;
	std::string mScenes_path;
	engine::fvector mScreen_size;
	engine::controls mKey_bindings;
	float pUnit_pixels;

	std::string load_setting_path(tinyxml2::XMLElement* pRoot, const std::string& pName, const std::string& pDefault);
};

}
#endif // !RPG_GAME_SETTINGS_LOADER_HPP
