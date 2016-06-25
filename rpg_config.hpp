#ifndef RPG_CONFIG_HPP
#define RPG_CONFIG_HPP

#include "vector.hpp"
#include "time.hpp"

namespace rpg
{

const int TILE_WIDTH_PX = 32;
const int TILE_HEIGHT_PX = 32;
const engine::ivector TILE_SIZE = { TILE_WIDTH_PX, TILE_HEIGHT_PX };
const int TILE_DEPTH_RANGE_MIN = 1;
const int TILE_DEPTH_RANGE_MAX = 100;
const engine::time_t DEFAULT_DIALOG_SPEED = 30;
const size_t CONTROL_COUNT = 8;
const engine::time_t FADE_DURATION = 0.2;

}
#endif