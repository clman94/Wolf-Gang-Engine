#pragma once

#include <wge/core/behavior_component.hpp>
#include <wge/core/behavior.hpp>
#include <wge/core/system.hpp>
#include <wge/core/layer.hpp>

namespace wge::core
{

class behavior_system :
	public system
{
	WGE_SYSTEM("Behavior", 40392);
public:
	behavior_system(layer& pLayer) :
		system(pLayer)
	{}

	virtual void update(float pDelta) override
	{
		get_layer().for_each(
			[&](game_object pObject, behavior_component& pBehavior)
		{
			if (auto instance = pBehavior.get_behavior_instance())
			{
				instance->on_update(pDelta, pObject);
			}
		});
	}
};

} // namespace wge::core
