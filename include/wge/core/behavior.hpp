#pragma once

#include <wge/core/asset.hpp>
#include <wge/core/component.hpp>
#include <wge/core/behavior.hpp>

#include <memory>
#include <functional>

namespace wge::core
{

class behavior_instance
{
public:
	using ptr = std::shared_ptr<behavior_instance>;

	virtual ~behavior_instance() {}

	virtual json serialize(serialize_type) { return{}; }
	virtual void deserialize(const json&) {}

	virtual void on_update(float pDelta, game_object pObject) {}
};

class behavior :
	public asset
{
public:
	using ptr = std::shared_ptr<behavior>;
	using instance_factory_function = std::function<behavior_instance::ptr(const behavior&)>;

	behavior(asset_config::ptr pConfig) :
		asset(pConfig)
	{}

	void set_instance_factory(const instance_factory_function& pFactory) noexcept
	{
		mFactory = pFactory;
	}

	behavior_instance::ptr create_instance() const
	{
		if (mFactory)
			return mFactory(*this);
		else
			return{};
	}

private:
	instance_factory_function mFactory;
};

} // namespace wge::scripting::behavior
