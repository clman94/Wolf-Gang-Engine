#pragma once

#include <wge/math/vector.hpp>

// Predefined ratios
namespace wge::math::anchor
{

static constexpr math::vec2 top        { 0.5f, 0 };
static constexpr math::vec2 topleft    { 0, 0 };
static constexpr math::vec2 topright   { 1, 0 };
static constexpr math::vec2 bottom     { 0.5f, 1 };
static constexpr math::vec2 bottomleft { 0, 1 };
static constexpr math::vec2 bottomright{ 1, 1 };
static constexpr math::vec2 left       { 0, 0.5f };
static constexpr math::vec2 right      { 1, 0.5f };
static constexpr math::vec2 center     { 0.5f, 0.5f };

} // namespace wge::math::anchor