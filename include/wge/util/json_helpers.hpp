#pragma once

#include <nlohmann/json.hpp>

#include <wge/util/strongly_typed_id.hpp>

#include <string_view>
#include <optional>

namespace wge
{

using nlohmann::json;

} // namespace wge

namespace wge::util
{

template <typename T>
inline std::optional<T> optional_deserialize(const json& pJson)
{
	if (pJson.find(pItem) != pJson.end())
		return static_cast<T>(pJson);
	return{};
}

// Deserialize json value only if it exists.
// Returns true if the item has been deserialized.
template <typename T>
inline bool optional_deserialize(const json& pJson, const std::string_view& pItem, T& pDestination)
{
	if (pJson.find(pItem) != pJson.end())
	{
		pDestination = pJson[pItem];
		return true;
	}
	return false;
}

template <typename T, typename Tdefault>
inline bool optional_deserialize(const json& pJson, const std::string_view& pItem, T& pDestination, Tdefault&& pDefault)
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

class transform;
void to_json(nlohmann::json&, const transform&);
void from_json(const nlohmann::json&, transform&);

} // namespace wge::math

namespace wge::util
{

template <typename T, typename Tvalue>
void to_json(nlohmann::json& pJson, const strongly_typed_id<T, Tvalue>& pId)
{
	pJson = pId.get_value();
}
template <typename T, typename Tvalue>
void from_json(const nlohmann::json& pJson, strongly_typed_id<T, Tvalue>& pId)
{
	pId.set_value(pJson);
}

class uuid;
void to_json(nlohmann::json&, const uuid&);
void from_json(const nlohmann::json&, uuid&);

} // namespace wge::util

struct b2Vec2;
void to_json(nlohmann::json&, const b2Vec2&);
void from_json(const nlohmann::json&, b2Vec2&);

namespace wge::filesystem
{

class path;
void to_json(nlohmann::json&, const path&);
void from_json(const nlohmann::json&, path&);

} // namespace wge::filesystem
