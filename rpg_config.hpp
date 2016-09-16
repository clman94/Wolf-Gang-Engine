#ifndef RPG_CONFIG_HPP
#define RPG_CONFIG_HPP

#include "vector.hpp"
#include "time.hpp"

namespace rpg{ namespace defs{

const engine::fvector DISPLAY_SIZE           = { 320, 256 };
const engine::fvector SCREEN_SIZE            = { DISPLAY_SIZE.x * 3, DISPLAY_SIZE.y * 3 };
const int             TILE_WIDTH_PX          = 32;
const int             TILE_HEIGHT_PX         = 32;
const engine::ivector TILE_SIZE              = { TILE_WIDTH_PX, TILE_HEIGHT_PX };
const engine::time_t  DEFAULT_DIALOG_SPEED   = 30;
const size_t          DIALOG_FX_INTERVAL     = 2;
const size_t          CONTROL_COUNT          = 8;
const engine::time_t  FADE_DURATION          = 0.2f;

const float           TILES_DEPTH = 103;
const float           TILE_DEPTH_RANGE_MIN   = 1;
const float           TILE_DEPTH_RANGE_MAX = 100;
const float           NARRATIVE_TEXT_DEPTH = -3;
const float           NARRATIVE_BOX_DEPTH = -2;
const float           FX_DEPTH = -1;
const float           ABSOLUTE_OVERLAP_DEPTH = -100000;
}
}
#endif