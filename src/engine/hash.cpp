#include <engine/hash.hpp>

#include <cstring>

namespace hash
{

hash32_t FNV1a_32(const std::uint8_t* pData, std::size_t pSize)
{
	const hash32_t basis = 0x811C9DC5;
	const hash32_t prime = 0x01000193;
	hash32_t h = basis;
	for (std::size_t i = 0; i < pSize; i++)
	{
		h ^= pData[i];
		h *= prime;
	}
	return h;
}

hash32_t FNV1a_32(const char* pString, int pLength)
{
	if (pLength < 0)
		pLength = std::strlen(pString);
	return FNV1a_32((const std::uint8_t*)pString, static_cast<std::size_t>(pLength));
}

hash32_t FNV1a_32(const std::string& pString)
{
	return FNV1a_32(pString.c_str(), pString.size());
}

hash32_t combine(const hash32_t& pA, const hash32_t& pB)
{
	return pA ^ pB;
}

}