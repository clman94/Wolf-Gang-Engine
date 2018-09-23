#include <wge/util/json_helpers.hpp>

#include <nlohmann/json.hpp>
using nlohmann::json;

#include <Box2D/Common/b2Math.h>

#include <wge/math/math.hpp>
#include <wge/math/vector.hpp>
#include <wge/math/rect.hpp>
using namespace wge;

void wge::math::to_json(nlohmann::json & pJson, const math::vec2 & pVec)
{
	pJson = { pVec.x, pVec.y };
}

void wge::math::from_json(const nlohmann::json & pJson, math::vec2 & pVec)
{
	pVec.x = pJson[0];
	pVec.y = pJson[1];
}

void wge::math::to_json(nlohmann::json & pJson, const radians & pRad)
{
	pJson = pRad.value();
}

void wge::math::from_json(const nlohmann::json & pJson, radians & pRad)
{
	pRad = static_cast<float>(pJson);
}

void wge::math::to_json(nlohmann::json & pJson, const degrees & pDeg)
{
	pJson = pDeg.value();
}

void wge::math::from_json(const nlohmann::json & pJson, degrees & pDeg)
{
	pDeg = static_cast<float>(pJson);
}

void wge::math::to_json(nlohmann::json & pJson, const rect & pRect)
{
	pJson = { pRect.x, pRect.y, pRect.width, pRect.height };
}

void wge::math::from_json(const nlohmann::json & pJson, rect & pRect)
{
	for (int i = 0; i < 4; i++)
		pRect.components[i] = pJson[i];
}


void to_json(nlohmann::json & pJson, const b2Vec2 & pVec)
{
	pJson = { pVec.x, pVec.y };
}

void from_json(const nlohmann::json & pJson, b2Vec2 & pVec)
{
	pVec.x = pJson[0];
	pVec.y = pJson[1];
}
