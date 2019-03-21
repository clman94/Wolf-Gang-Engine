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

	virtual void on_update(float pDelta, const game_object& pObject) {}
};

class behavior :
	public asset
{
public:
	using ptr = std::shared_ptr<behavior>;

	behavior(asset_config::ptr pConfig) :
		asset(pConfig)
	{}

	virtual ~behavior() {}

	virtual behavior_instance::ptr create_instance() const = 0;
};

} // namespace wge::scripting::behavior
