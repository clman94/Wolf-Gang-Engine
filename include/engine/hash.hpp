#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace hash
{

typedef std::uint32_t hash32_t;

hash32_t FNV1a_32(const std::uint8_t* pData, std::size_t pSize);
hash32_t FNV1a_32(const char* pString, int pLength = -1); // pString must be null terminated if length < 0
hash32_t FNV1a_32(const std::string& pString);

hash32_t combine(const hash32_t& pA, const hash32_t& pB);

}