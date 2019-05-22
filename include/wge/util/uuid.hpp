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
	uuid() = default;

	uuid(const std::string_view& pStr);

	template <typename Titer>
	uuid(Titer pBegin, Titer pEnd)
	{
		if (std::distance(pBegin, pEnd) == 16)
			std::copy(pBegin, pEnd, mBytes.begin());
	}

	bool operator < (const uuid& pR) const noexcept;
	bool operator == (const uuid& pR) const noexcept;
	bool operator != (const uuid& pR) const noexcept;

	// Returns the difference between the first bytes of both uuids that differ.
	int compare(const uuid& pR) const noexcept;

	std::string to_string() const;

	bool parse(const std::string_view& pStr);

	json to_json() const;
	void from_json(const json& pJson);

	hash::hash32_t to_hash32() const noexcept;

	// Returns true when all values are 0
	bool is_valid() const noexcept;

private:
	std::array<std::uint8_t, 16> mBytes{ { 0 } };
};

uuid generate_uuid();

} // namespace wge::util
