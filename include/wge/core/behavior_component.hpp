#pragma once

#include <wge/core/component.hpp>
#include <wge/core/behavior.hpp>

namespace wge::core
{

class behavior_component :
	public component
{
	WGE_COMPONENT("Behavior", 9432);
public:
	behavior_component(component_id pId) noexcept :
		component(pId)
	{}

	void set_behavior(const behavior::ptr& pBehavior) noexcept
	{
		mBehavior = pBehavior;
	}

	behavior::ptr get_behavior() const noexcept
	{
		return mBehavior;
	}

	void reset_instance() noexcept
	{
		mInstance.reset();
	}

	behavior_instance::ptr get_behavior_instance()
	{
		if (!mInstance && mBehavior)
			return mInstance = mBehavior->create_instance();
		else
			return mInstance;
	}

private:
	behavior::ptr mBehavior;
	behavior_instance::ptr mInstance;
};

} // namespace wge::scripting::behavior
