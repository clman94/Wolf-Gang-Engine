#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace wge::util::hash
{

typedef std::uint32_t hash32_t;
typedef std::uint64_t hash64_t;

hash32_t hash32(const std::uint8_t* pData, std::size_t pSize);
hash32_t hash32(const char* pString, int pLength = -1); // pString must be null terminated if length < 0
hash32_t hash32(const std::string& pString);

hash64_t hash64(const std::uint8_t* pData, std::size_t pSize);
hash64_t hash64(const char* pString, int pLength = -1); // pString must be null terminated if length < 0
hash64_t hash64(const std::string& pString);

hash32_t combine(const hash32_t& pA, const hash32_t& pB);
hash64_t combine(const hash64_t& pA, const hash64_t& pB);

}