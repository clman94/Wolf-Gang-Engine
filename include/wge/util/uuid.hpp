#pragma once

#include <array>
#include <string>
#include <algorithm>
#include <cstdint>

#include <wge/util/json_helpers.hpp>
#include <wge/util/hash.hpp>

namespace wge::util
{

class uuid
{
public:
	constexpr uuid() noexcept = default;
	uuid(const std::string_view& pStr);

	template <typename Titer>
	constexpr uuid(Titer pBegin, Titer pEnd)
	{
		if (std::distance(pBegin, pEnd) == 16)
			std::copy(pBegin, pEnd, mBytes.begin());
	}

	// Returns the difference between the first bytes of both uuids that differ.
	constexpr int compare(const uuid& pR) const noexcept
	{
		for (std::size_t i = 0; i < 16; ++i)
			if (mBytes[i] != pR.mBytes[i])
				return static_cast<int>(mBytes[i]) - static_cast<int>(pR.mBytes[i]);
		return 0;
	}

	constexpr bool operator < (const uuid& pR) const noexcept { return compare(pR) < 0; }
	constexpr bool operator > (const uuid& pR) const noexcept { return compare(pR) > 0; }
	constexpr bool operator == (const uuid& pR) const noexcept { return compare(pR) == 0; }
	constexpr bool operator != (const uuid& pR) const noexcept { return compare(pR) != 0; }
	constexpr bool operator <= (const uuid& pR) const noexcept { return compare(pR) <= 0; }
	constexpr bool operator >= (const uuid& pR) const noexcept { return compare(pR) >= 0; }

	std::string to_string() const;
	// Outputs only the last 12 digits of the uuid.
	std::string to_shortened_string() const;

	bool parse(const std::string_view& pStr);

	json to_json() const;
	void from_json(const json& pJson);

	constexpr hash::hash32_t to_hash32() const noexcept
	{
		return hash::hash32(mBytes.begin(), mBytes.end());
	}

	constexpr hash::hash64_t to_hash64() const noexcept
	{
		return hash::hash64(mBytes.begin(), mBytes.end());
	}

	// Returns false when all values are 0.
	constexpr bool is_valid() const noexcept
	{
		for (auto i : mBytes)
			if (i != 0)
				return true;
		return false;
	}


	constexpr std::uint8_t& operator[](std::size_t pIndex)
	{
		return mBytes[pIndex];
	}

	constexpr std::uint8_t operator[](std::size_t pIndex) const
	{
		return mBytes[pIndex];
	}

	constexpr auto begin() noexcept
	{
		return mBytes.begin();
	}

	constexpr auto end() noexcept
	{
		return mBytes.end();
	}

private:
	std::array<std::uint8_t, 16> mBytes{ { 0 } };
};

uuid generate_uuid();

} // namespace wge::util

namespace std
{
template<> struct hash<wge::util::uuid>
{
	std::size_t operator()(const wge::util::uuid& pU) const noexcept
	{
		if constexpr (sizeof(std::size_t) == 4)
			return pU.to_hash32();
		else if constexpr(sizeof(std::size_t) == 8)
			return pU.to_hash64();
	}
};
}
