#pragma once

#include <type_traits>
#include <memory>

namespace wge::util
{

template <typename Tto, typename Tfrom>
constexpr std::unique_ptr<Tto> dynamic_unique_cast(std::unique_ptr<Tfrom> pFrom) noexcept
{
	auto ptr = pFrom.release();
	if (auto cast = dynamic_cast<Tto*>(ptr))
	{
		return std::unique_ptr<Tto>(cast);
	}
	return nullptr;
}

template <typename Tto, typename Tfrom>
constexpr std::unique_ptr<Tto> static_unique_cast(std::unique_ptr<Tfrom> pFrom) noexcept
{
	return std::unique_ptr<Tto>(static_cast<Tto*>(pFrom.release()));
}

// Obtain raw pointer from pointer-like objects (shared_ptr, unique_ptr...)
template <typename T>
inline constexpr auto to_address(T&& pPtr)
{
	return &(*pPtr);
}

} // namespace wge::util
