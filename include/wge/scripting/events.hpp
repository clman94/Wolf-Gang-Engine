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

template <std::size_t Tbucket>
using bselect = core::bselect<event_component, Tbucket>;

using create = bselect<0>;
using destroy = bselect<1>;
using preupdate = bselect<2>;
using update = bselect<3>;
using postupdate = bselect<4>;
using draw = bselect<5>;

using unique_create = bselect<6>;

using alarm_1 = bselect<7>;
using alarm_2 = bselect<8>;
using alarm_3 = bselect<9>;
using alarm_4 = bselect<10>;
using alarm_5 = bselect<11>;
using alarm_6 = bselect<12>;
using alarm_7 = bselect<13>;
using alarm_8 = bselect<14>;

// Planned events

// Physics Events:
// Start collision
// Continued Collision
// End Collision

// Sprite Events:
// New Frame
// Begin Animation
// End Animation

// Editor/Debug Events:
// Debug Gui

// Load/Save:
// Load
// Save

constexpr core::bucket bucket_count = 15;

} // namespace event_selectors

struct event_descriptor
{
	const char* serialize_name;
	const char* display_name;
	const char* icon;
	core::bucket bucket;
	const char* tip;
};

constexpr std::array event_descriptors =
{
	event_descriptor{ "create",        "Create",        u8"\uf067",  event_selector::create::bucket, "Triggered only once at the the beginning of the frame after the object has been created." },
	event_descriptor{ "destroy",       "Destroy",       u8"\uf067",  event_selector::destroy::bucket, "Triggered before the object is deleted from the scene." },
	event_descriptor{ "unique_create", "Unique Create", "",          event_selector::unique_create::bucket, "" },
	event_descriptor{ "preupdate",     "Pre-Update",    u8"\uf051",  event_selector::preupdate::bucket, "Event that precedes the Update event. Only use if you require some setup before all Update events are triggered."},
	event_descriptor{ "update",        "Update",        u8"\uf051",  event_selector::update::bucket, "Triggered every frame. Normal game logic should go here." },
	event_descriptor{ "postupdate",    "Post-Update",   u8"\uf051",  event_selector::postupdate::bucket, "Event that follows the Update event. Only use if you require some logic after all Update events are triggered." },
	event_descriptor{ "draw",          "Draw",          u8"\uf040",  event_selector::draw::bucket, "Custom rendering."},

	event_descriptor{ "alarm_1",       "Alarm 1",       u8"\uf017",  event_selector::alarm_1::bucket, "Triggered when the variable 'alarm[1]' is equal or below 0." },
	event_descriptor{ "alarm_2",       "Alarm 2",       u8"\uf017",  event_selector::alarm_2::bucket, "Triggered when the variable 'alarm[2]' is equal or below 0." },
	event_descriptor{ "alarm_3",       "Alarm 3",       u8"\uf017",  event_selector::alarm_3::bucket, "Triggered when the variable 'alarm[3]' is equal or below 0." },
	event_descriptor{ "alarm_4",       "Alarm 4",       u8"\uf017",  event_selector::alarm_4::bucket, "Triggered when the variable 'alarm[4]' is equal or below 0." },
	event_descriptor{ "alarm_5",       "Alarm 5",       u8"\uf017",  event_selector::alarm_5::bucket, "Triggered when the variable 'alarm[5]' is equal or below 0." },
	event_descriptor{ "alarm_6",       "Alarm 6",       u8"\uf017",  event_selector::alarm_6::bucket, "Triggered when the variable 'alarm[6]' is equal or below 0." },
	event_descriptor{ "alarm_7",       "Alarm 7",       u8"\uf017",  event_selector::alarm_7::bucket, "Triggered when the variable 'alarm[7]' is equal or below 0." },
	event_descriptor{ "alarm_8",       "Alarm 8",       u8"\uf017",  event_selector::alarm_8::bucket, "Triggered when the variable 'alarm[8]' is equal or below 0." },
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
