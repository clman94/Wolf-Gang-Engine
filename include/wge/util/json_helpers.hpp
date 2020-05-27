#pragma once

#include <nlohmann/json.hpp>

#include <wge/math/vector.hpp>

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

template <typename T>
void to_json(nlohmann::json& pJson, const math::basic_vec2<T>& pVec)
{
	pJson = { pVec.x, pVec.y };
}

template <typename T>
void from_json(const nlohmann::json& pJson, math::basic_vec2<T>& pVec)
{
	pVec.x = pJson[0];
	pVec.y = pJson[1];
}
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

namespace nlohmann
{

template <typename T>
struct adl_serializer<std::optional<T>>
{
	static void to_json(json& pJson, const std::optional<T>& pOpt)
	{
		if (pOpt.has_value())
			pJson = pOpt.value();
		else 
			pJson = nullptr;
	}

	static void from_json(const json& pJson, std::optional<T>& pOpt)
	{
		if (pJson.is_null())
			pOpt = {};
		else
			pOpt = pJson.get<T>();
	}
};

} // namespace nlohmann


