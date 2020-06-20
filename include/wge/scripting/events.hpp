#pragma once

#include <wge/core/component_type.hpp>
#include <wge/math/vector.hpp>
#include <wge/scripting/script.hpp>

#include <sol/environment.hpp>

#include <string>
#include <variant>

namespace wge::scripting
{

class event_state_component
{
public:
	using property_value = std::variant<int, float, math::vec2, std::string>;
	struct property
	{
		std::string name;
		property_value value{ 0 };
	};
	std::vector<property> properties;
	sol::environment environment;
};

class event_component
{
public:
	script::handle source_script;
	const std::string& get_source() const
	{
		WGE_ASSERT(source_script.is_valid());
		return source_script->source;
	}
};

namespace event_selector
{

using create = core::bselect<event_component, 0>;
using preupdate = core::bselect<event_component, 1>;
using update = core::bselect<event_component, 2>;
using postupdate = core::bselect<event_component, 3>;
using draw = core::bselect<event_component, 4>;

constexpr core::bucket bucket_count = 5;

} // namespace event_selectors

} // namespace wge::scripting
