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

template <typename T, typename Tkey, typename Tdefault>
inline T json_get_or(const json& pJson, const Tkey& pKey, Tdefault&& pDefault)
{
	static_assert(!std::is_reference_v<T>, "T cannot be a reference");
	auto iter = pJson.find(pKey);
	if (iter != pJson.end())
		return iter->get<T>();
	else
		return pDefault;
}

template <typename T, typename Tjson, typename Tkey, typename Tdefault>
inline T json_get_or_safe(const json& pJson, const Tkey& pKey, Tdefault&& pDefault)
{
	try {
		return json_get_or<T>(pJson, pKey, std:forward<Tdefault>(pDefault));
	}
	catch (const json::exception& e)
	{
		return pDefault;
	}
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
inline void to_json(nlohmann::json& pJson, const strongly_typed_id<T, Tvalue>& pId)
{
	pJson = pId.get_value();
}
template <typename T, typename Tvalue>
inline void from_json(const nlohmann::json& pJson, strongly_typed_id<T, Tvalue>& pId)
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
			pOpt = std::nullopt;
		else
			pOpt = pJson.get<T>();
	}
};

} // namespace nlohmann


