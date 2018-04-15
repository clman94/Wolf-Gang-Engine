#ifndef RPG_CONFIG_HPP
#define RPG_CONFIG_HPP

#include <engine/vector.hpp>
#include <engine/time.hpp>
#include <engine/filesystem.hpp>

#include <string>

namespace rpg{
namespace defs{

const engine::fvector DISPLAY_SIZE = engine::fvector(32, 32)*engine::fvector(10, 8); // 10x8 of 32x32 pixel tiles
const engine::fvector SCREEN_SIZE            = { DISPLAY_SIZE.x * 2, DISPLAY_SIZE.y * 2 };
const float           DEFAULT_DIALOG_SPEED   = 30;

const engine::fs::path DEFAULT_DATA_PATH      = "./data";
const engine::fs::path DEFAULT_AUDIO_PATH     = "audio";
const engine::fs::path DEFAULT_SAVES_PATH     = "saves";
const engine::fs::path DEFAULT_TEXTURES_PATH  = "textures";
const engine::fs::path DEFAULT_SCENES_PATH    = "scenes";
const engine::fs::path DEFAULT_INTERNALS_PATH = "internal";
const engine::fs::path DEFAULT_FONTS_PATH     = "fonts";

const engine::fs::path INTERNAL_SCRIPTS_PATH = DEFAULT_INTERNALS_PATH / "scene.as";
const std::string INTERNAL_SCRIPTS_INCLUDE   = "#include \"" + (defs::DEFAULT_DATA_PATH / INTERNAL_SCRIPTS_PATH).string() + "\"";

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
"\t//set_position(player::get(), vec(0, 0));\n"
"}\n";

}
}
#endif