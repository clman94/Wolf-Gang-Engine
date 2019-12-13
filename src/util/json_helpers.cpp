#include <wge/util/json_helpers.hpp>

#include <Box2D/Common/b2Math.h>

#include <wge/math/math.hpp>
#include <wge/math/vector.hpp>
#include <wge/math/rect.hpp>
#include <wge/math/transform.hpp>
#include <wge/util/uuid.hpp>
#include <wge/filesystem/path.hpp>

using namespace wge;

void wge::math::to_json(nlohmann::json& pJson, const radians& pRad)
{
	pJson = pRad.value();
}

void wge::math::from_json(const nlohmann::json& pJson, radians& pRad)
{
	pRad = static_cast<float>(pJson);
}

void wge::math::to_json(nlohmann::json& pJson, const degrees& pDeg)
{
	pJson = pDeg.value();
}

void wge::math::from_json(const nlohmann::json& pJson, degrees& pDeg)
{
	pDeg = static_cast<float>(pJson);
}

void wge::math::to_json(nlohmann::json& pJson, const rect& pRect)
{
	pJson = { pRect.x, pRect.y, pRect.width, pRect.height };
}

void wge::math::from_json(const nlohmann::json& pJson, rect& pRect)
{
	for (int i = 0; i < 4; i++)
		pRect.components[i] = pJson[i];
}

void wge::math::to_json(nlohmann::json& pJson, const transform& pTransform)
{
	pJson["position"] = pTransform.position;
	pJson["rotation"] = pTransform.rotation;
	pJson["scale"] = pTransform.scale;
}

void wge::math::from_json(const nlohmann::json& pJson, transform& pTransform)
{
	pTransform.position = pJson["position"];
	pTransform.rotation = pJson["rotation"];
	pTransform.scale = pJson["scale"];
}

void to_json(nlohmann::json& pJson, const b2Vec2& pVec)
{
	pJson = { pVec.x, pVec.y };
}

void from_json(const nlohmann::json& pJson, b2Vec2& pVec)
{
	pVec.x = pJson[0];
	pVec.y = pJson[1];
}

void wge::util::to_json(nlohmann::json& pJson, const uuid& pUuid)
{
	pJson = pUuid.to_json();
}

void wge::util::from_json(const nlohmann::json& pJson, uuid& pUuid)
{
	pUuid.from_json(pJson);
}

void wge::filesystem::to_json(nlohmann::json& pJson, const path& pPath)
{
	pJson = pPath.string();
}

void wge::filesystem::from_json(const nlohmann::json& pJson, path& pPath)
{
	pPath.parse(pJson);
}
