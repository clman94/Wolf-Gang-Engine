#pragma once

#include <wge/core/component_type.hpp>
#include <wge/math/vector.hpp>
#include <wge/scripting/script.hpp>

#include <sol/environment.hpp>

#include <string>
#include <string_view>
#include <variant>
#include <array>
#include <unordered_map>

namespace wge::scripting
{

class event_state_component
{
public:
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

using unique_create = core::bselect<event_component, 5>;

using alarm_1 = core::bselect<event_component, 6>;
using alarm_2 = core::bselect<event_component, 7>;
using alarm_3 = core::bselect<event_component, 8>;
using alarm_4 = core::bselect<event_component, 9>;
using alarm_5 = core::bselect<event_component, 10>;
using alarm_6 = core::bselect<event_component, 11>;
using alarm_7 = core::bselect<event_component, 12>;
using alarm_8 = core::bselect<event_component, 13>;

constexpr core::bucket bucket_count = 14;

} // namespace event_selectors

struct event_descriptor
{
	const char* serialize_name;
	const char* display_name;
	const char* icon;
	core::bucket bucket;
};

constexpr std::array event_descriptors =
{
	event_descriptor{ "create",        "Create",        u8"\uf067",  event_selector::create::bucket },
	event_descriptor{ "unique_create", "Unique Create", "",          event_selector::unique_create::bucket },
	event_descriptor{ "preupdate",     "Pre-Update",    u8"\uf051",  event_selector::preupdate::bucket },
	event_descriptor{ "update",        "Update",        u8"\uf051",  event_selector::update::bucket },
	event_descriptor{ "postupdate",    "Post-Update",   u8"\uf051",  event_selector::postupdate::bucket },
	event_descriptor{ "draw",          "Draw",          u8"\uf040",  event_selector::draw::bucket },

	event_descriptor{ "alarm_1",       "Alarm 1",       u8"\uf017",  event_selector::alarm_1::bucket },
	event_descriptor{ "alarm_2",       "Alarm 2",       u8"\uf017",  event_selector::alarm_2::bucket },
	event_descriptor{ "alarm_3",       "Alarm 3",       u8"\uf017",  event_selector::alarm_3::bucket },
	event_descriptor{ "alarm_4",       "Alarm 4",       u8"\uf017",  event_selector::alarm_4::bucket },
	event_descriptor{ "alarm_5",       "Alarm 5",       u8"\uf017",  event_selector::alarm_5::bucket },
	event_descriptor{ "alarm_6",       "Alarm 6",       u8"\uf017",  event_selector::alarm_6::bucket },
	event_descriptor{ "alarm_7",       "Alarm 7",       u8"\uf017",  event_selector::alarm_7::bucket },
	event_descriptor{ "alarm_8",       "Alarm 8",       u8"\uf017",  event_selector::alarm_8::bucket }
};

constexpr std::size_t get_event_descriptor_index(std::size_t pBucket) noexcept
{
	std::size_t index = 0;
	for (auto& i : event_descriptors)
	{
		if (i.bucket == pBucket)
			return index;
		++index;
	}
	return 0;
}

template <std::size_t Tbucket>
constexpr std::size_t get_event_descriptor_index(core::bselect<event_component, Tbucket>) noexcept
{
	static_assert(Tbucket < event_selector::bucket_count, "No descriptor for this bucket type");
	return get_event_descriptor_index(Tbucket);
}

template <std::size_t Tbucket>
constexpr const event_descriptor* get_event_descriptor(core::bselect<event_component, Tbucket>) noexcept
{
	static_assert(Tbucket < event_selector::bucket_count, "No descriptor for this bucket type");
	for (auto& i : event_descriptors)
		if (i.bucket == Tbucket)
			return &i;
	return nullptr; // Should not happen.
}

constexpr std::size_t event_count = event_descriptors.size();

} // namespace wge::scripting
