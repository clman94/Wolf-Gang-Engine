#ifndef RPG_CONFIG_HPP
#define RPG_CONFIG_HPP

#include <engine/vector.hpp>
#include <engine/time.hpp>
#include <engine/filesystem.hpp>

#include <string>

namespace rpg{
namespace defs{

const engine::fvector DISPLAY_SIZE           = { 320, 256 };
const engine::fvector SCREEN_SIZE            = { DISPLAY_SIZE.x * 2, DISPLAY_SIZE.y * 2 };
const engine::time_t  DEFAULT_DIALOG_SPEED   = 30;

const engine::fs::path DEFAULT_DATA_PATH      = "./data";
const engine::fs::path DEFAULT_MUSIC_PATH     = DEFAULT_DATA_PATH / "music";
const engine::fs::path DEFAULT_SOUND_PATH     = DEFAULT_DATA_PATH / "sounds";
const engine::fs::path DEFAULT_SAVES_PATH     = DEFAULT_DATA_PATH / "saves";
const engine::fs::path DEFAULT_TEXTURES_PATH  = DEFAULT_DATA_PATH / "textures";
const engine::fs::path DEFAULT_SCENES_PATH    = DEFAULT_DATA_PATH / "scenes";
const engine::fs::path DEFAULT_INTERNALS_PATH = DEFAULT_DATA_PATH / "internal";
const engine::fs::path DEFAULT_FONTS_PATH     = DEFAULT_DATA_PATH / "fonts";

const engine::fs::path INTERNAL_SCRIPTS_PATH = DEFAULT_INTERNALS_PATH / "scene.as";
const std::string INTERNAL_SCRIPTS_INCLUDE   = "#include \"" + INTERNAL_SCRIPTS_PATH.string() + "\"";

const float TILES_DEPTH            = 103;
const float TILE_DEPTH_RANGE_MIN   = 1;
const float TILE_DEPTH_RANGE_MAX   = 100;
const float GUI_DEPTH              = -2;
const float FX_DEPTH               = -1;
const float ABSOLUTE_OVERLAP_DEPTH = -100000;

const std::string MINIMAL_XML_SCENE =
	"<scene>\n"
	"\t<map><texture/></map>\n"
	"\t<collisionboxes/>\n"
	"</scene>";

const std::string MINIMAL_SCRIPT_SCENE =
	"[start]\n"
	"void start()\n"
	"{\n"
	"\t//set_position(get_player(), vec(0, 0))\n"
	"}\n";

}
}
#endif