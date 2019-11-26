#pragma once

#include <type_traits>
#include <memory>

namespace wge::util
{

// Obtain raw pointer from pointer-like objects (shared_ptr, unique_ptr...)
template <typename T>
inline constexpr auto to_address(T&& pPtr)
{
	return &(*pPtr);
}

} // namespace wge::util
