#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace wge::util::hash
{

using hash32_t = std::uint32_t;
using hash64_t = std::uint64_t;

// FNV1a implementation
// Hashes an array of bytes.
template <typename Iiter>
constexpr hash32_t hash32(Iiter pBegin, Iiter pEnd) noexcept
{
	constexpr hash32_t basis = 0x811C9DC5;
	constexpr hash32_t prime = 0x01000193;
	hash32_t h = basis;
	for (Iiter i = pBegin; i != pEnd; i++)
	{
		h ^= *i;
		h *= prime;
	}
	return h;
}

constexpr hash32_t hash32(std::string_view pString) noexcept
{
	return hash32(pString.begin(), pString.end());
}

// FNV1a implementation
// Hashes an array of bytes.
template <typename Iiter>
constexpr hash64_t hash64(Iiter pBegin, Iiter pEnd) noexcept
{
	constexpr hash64_t basis = 0xcbf29ce484222325;
	constexpr hash64_t prime = 0x100000001B3;
	hash64_t h = basis;
	for (Iiter i = pBegin; i != pEnd; i++)
	{
		h ^= *i;
		h *= prime;
	}
	return h;
}

constexpr hash64_t hash64(std::string_view pString) noexcept
{
	return hash64(pString.begin(), pString.end());
}

constexpr hash32_t combine(const hash32_t& pA, const hash32_t& pB) noexcept
{
	return pA ^ pB;
}

constexpr hash64_t combine(const hash64_t& pA, const hash64_t& pB) noexcept
{
	return pA ^ pB;
}

} // namespace wge::util::hash
