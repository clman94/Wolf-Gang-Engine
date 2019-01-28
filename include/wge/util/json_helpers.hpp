#pragma once

#include <nlohmann/json.hpp>

#include <wge/core/instance_id.hpp>

#include <string_view>

namespace wge
{

using nlohmann::json;

} // namespace wge

namespace wge::util
{

// Deserialize json value only if it exists.
// Returns true if the item has been deserialized.
template <typename T>
inline bool optional_deserialize(const wge::json& pJson, const std::string_view& pItem, T& pDestination)
{
	if (pJson.find(pItem) != pJson.end())
	{
		pDestination = pJson[pItem];
		return true;
	}
	return false;
}

template <typename T, typename Tdefault>
inline bool optional_deserialize(const wge::json& pJson, const std::string_view& pItem, T& pDestination, Tdefault&& pDefault)
{
	if (!optional_deserialize(pJson, pItem, pDestination))
	{
		pDestination = pDefault;
		return false;
	}
	return true;
}

} // namespace wge::util

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

class rect;
void to_json(nlohmann::json&, const rect&);
void from_json(const nlohmann::json&, rect&);

} // namespace wge::math

namespace wge::core
{

template <typename T>
void to_json(nlohmann::json& pJson, const instance_id<T>& pId)
{
	pJson = pId.get_value();
}
template <typename T>
void from_json(const nlohmann::json& pJson, instance_id<T>& pId)
{
	pId.set_value(pJson);
}

} // namespace wge::core

struct b2Vec2;
void to_json(nlohmann::json&, const b2Vec2&);
void from_json(const nlohmann::json&, b2Vec2&);
