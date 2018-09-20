#pragma once

#include <nlohmann/json.hpp>
using nlohmann::json;

// To keep things small, we are going to use
// forward declarations as much as possible.

namespace wge::math
{

class vec2;
void to_json(nlohmann::json&, const vec2&);
void from_json(const nlohmann::json&, vec2&);

class radians;
void to_json(nlohmann::json&, const radians&);
void from_json(const nlohmann::json&, radians&);

class degrees;
void to_json(nlohmann::json&, const degrees&);
void from_json(const nlohmann::json&, degrees&);
}

struct b2Vec2;
void to_json(nlohmann::json&, const b2Vec2&);
void from_json(const nlohmann::json&, b2Vec2&);
