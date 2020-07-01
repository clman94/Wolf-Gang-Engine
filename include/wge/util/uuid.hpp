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
	constexpr uuid(std::string_view pStr) noexcept
	{
		parse(pStr);
	}

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

	constexpr bool parse(std::string_view pStr) noexcept;

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

namespace detail
{

constexpr std::uint8_t from_hex_char(char pC) noexcept
{
	if (pC >= '0' && pC <= '9')
		return static_cast<std::uint8_t>(pC - '0');
	else if (pC >= 'a' && pC <= 'f')
		return static_cast<std::uint8_t>(pC - 'a') + 10;
	else if (pC >= 'A' && pC <= 'F')
		return static_cast<std::uint8_t>(pC - 'A') + 10;
	else
		return 0;
}

constexpr std::uint8_t from_hex(std::string_view pSrc) noexcept
{
	if (pSrc.length() < 2)
		return 0;
	return (from_hex_char(pSrc[0]) << 4) + from_hex_char(pSrc[1]);
}

} // namespace detail

constexpr bool uuid::parse(std::string_view pStr) noexcept
{
	if (pStr.length() != 36)
		return false;

	mBytes[0] = detail::from_hex(pStr.substr(0, 2));
	mBytes[1] = detail::from_hex(pStr.substr(2, 2));
	mBytes[2] = detail::from_hex(pStr.substr(4, 2));
	mBytes[3] = detail::from_hex(pStr.substr(6, 2));

	mBytes[4] = detail::from_hex(pStr.substr(9, 2));
	mBytes[5] = detail::from_hex(pStr.substr(11, 2));

	mBytes[6] = detail::from_hex(pStr.substr(14, 2));
	mBytes[7] = detail::from_hex(pStr.substr(16, 2));

	mBytes[8] = detail::from_hex(pStr.substr(19, 2));
	mBytes[9] = detail::from_hex(pStr.substr(21, 2));

	mBytes[10] = detail::from_hex(pStr.substr(24, 2));
	mBytes[11] = detail::from_hex(pStr.substr(26, 2));
	mBytes[12] = detail::from_hex(pStr.substr(28, 2));
	mBytes[13] = detail::from_hex(pStr.substr(30, 2));
	mBytes[14] = detail::from_hex(pStr.substr(32, 2));
	mBytes[15] = detail::from_hex(pStr.substr(34, 2));

	return true;
}

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
		else if constexpr (sizeof(std::size_t) == 8)
			return pU.to_hash64();
	}
};
}
