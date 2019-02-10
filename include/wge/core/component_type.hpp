#pragma once

#include <wge/util/strongly_typed_id.hpp>
#include <cstdint>

namespace wge::core
{

struct _strong_component_type {};
using component_type = util::strongly_typed_id<_strong_component_type, std::uint64_t>;

} // namespace wge::core
