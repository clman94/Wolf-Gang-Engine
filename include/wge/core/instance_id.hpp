#pragma once

#include <wge/util/strongly_typed_id.hpp>
#include <cstdint>

namespace wge::core
{

// Type used for instance id values
using instance_id_t = std::uint64_t;

// Template for strongly typed id values.
template <typename T>
using instance_id = util::strongly_typed_id<T, instance_id_t>;

class game_object;
// Represents the instance id of a game object
using object_id = instance_id<game_object>;

class component;
// Represents the instance id of a component
using component_id = instance_id<component>;

} // namespace wge::core
