#include <wge/util/hash.hpp>

#include <cstring>

namespace wge::util::hash
{

// FNV1a implementation
hash32_t hash32(const std::uint8_t* pData, std::size_t pSize)
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

hash32_t hash32(const char* pString, int pLength)
{
	if (pLength < 0)
		pLength = std::strlen(pString);
	return hash32((const std::uint8_t*)pString, static_cast<std::size_t>(pLength));
}

hash32_t hash32(const std::string& pString)
{
	return hash32(pString.c_str(), pString.size());
}

hash32_t combine(const hash32_t& pA, const hash32_t& pB)
{
	return pA ^ pB;
}

// FNV1a implementation
hash64_t hash64(const std::uint8_t* pData, std::size_t pSize)
{
	const hash64_t basis = 0xcbf29ce484222325;
	const hash64_t prime = 0x100000001B3;
	hash64_t h = basis;
	for (std::size_t i = 0; i < pSize; i++)
	{
		h ^= pData[i];
		h *= prime;
	}
	return h;
}

hash64_t hash64(const char* pString, int pLength)
{
	if (pLength < 0)
		pLength = std::strlen(pString);
	return hash32((const std::uint8_t*)pString, static_cast<std::size_t>(pLength));
}

hash64_t hash64(const std::string& pString)
{
	return hash32(pString.c_str(), pString.size());
}

hash64_t combine(const hash64_t& pA, const hash64_t& pB)
{
	return pA ^ pB;
}

}