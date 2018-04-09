#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace hash
{

typedef std::uint32_t hash32_t;

hash32_t FNV1a_32(const std::uint8_t* pData, std::size_t pSize);
hash32_t FNV1a_32(const char* pData, int pLength = -1); // String must be null terminated if length < 0
hash32_t FNV1a_32(const std::string& pString);

}