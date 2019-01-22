#pragma once

#include <wge/util/enum.hpp>

namespace wge::core
{

enum class serialize_type : util::flag_type
{
	// Serialize runtime state. State that is created and
	// changed during runtime (usually by systems).
	// E.g. the current frame of an animation would be
	// runtime state.
	runtime_state = 1,

	// Serialize properties. Properties are created before
	// the runtime begins and are normally stored in assets.
	// However, properties can be manipulated during runtime.
	// E.g. the position of the frames of an animation in a texture.
	properties = 1 << 1,

	// Serialize all state that can be serialized.
	all = runtime_state | properties,
};
ENUM_CLASS_FLAG_OPERATORS(serialize_type);

} // namespace wge::core
