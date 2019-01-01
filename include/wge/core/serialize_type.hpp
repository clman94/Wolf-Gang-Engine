#pragma once

namespace wge::core
{

enum class serialize_type : unsigned int
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
	properties = 2,

	// Serialize all state that can be serialized.
	// Same as: runtime_state | properties
	all = 3,
};

inline bool operator & (const serialize_type& pL, const serialize_type& pR) noexcept
{
	return (static_cast<unsigned int>(pL) & static_cast<unsigned int>(pR)) != 0;
}

} // namespace wge::core