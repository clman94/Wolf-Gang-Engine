#include <wge/util/uuid.hpp>
#include <wge/logging/log.hpp>

#include <random>
#include <cctype>

namespace wge::util
{

static std::uint8_t from_hex_char(char pC)
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

static std::uint8_t from_hex(const char* pSrc)
{
	return (from_hex_char(pSrc[0]) << 4) + from_hex_char(pSrc[1]);
}

static void put_hex(char* pDest, std::uint8_t pVal)
{
	constexpr const char* chars = "0123456789abcdef";
	pDest[0] = chars[pVal >> 4];
	pDest[1] = chars[pVal & 0x0f];
}

inline uuid::uuid(const std::string_view& pStr)
{
	parse(pStr);
}

bool uuid::operator < (const uuid& pR) const noexcept
{
	return compare(pR) < 0;
}

bool uuid::operator == (const uuid& pR) const noexcept
{
	return compare(pR) == 0;
}

bool uuid::operator != (const uuid& pR) const noexcept
{
	return compare(pR) != 0;
}

int uuid::compare(const uuid& pR) const noexcept
{
	for (std::size_t i = 0; i < 16; ++i)
		if (mBytes[i] != pR.mBytes[i])
			return static_cast<int>(mBytes[i]) - static_cast<int>(pR.mBytes[i]);
	return 0;
}

std::string uuid::to_string() const
{
	std::string result;
	result.resize(36);

	put_hex(&result[0], mBytes[0]);
	put_hex(&result[2], mBytes[1]);
	put_hex(&result[4], mBytes[2]);
	put_hex(&result[6], mBytes[3]);
	result[8] = '-';
	put_hex(&result[9], mBytes[4]);
	put_hex(&result[11], mBytes[5]);
	result[13] = '-';
	put_hex(&result[14], mBytes[6]);
	put_hex(&result[16], mBytes[7]);
	result[18] = '-';
	put_hex(&result[19], mBytes[8]);
	put_hex(&result[21], mBytes[9]);
	result[23] = '-';
	put_hex(&result[24], mBytes[10]);
	put_hex(&result[26], mBytes[11]);
	put_hex(&result[28], mBytes[12]);
	put_hex(&result[30], mBytes[13]);
	put_hex(&result[32], mBytes[14]);
	put_hex(&result[34], mBytes[15]);

	return result;
}

std::string uuid::to_shortened_string() const
{
	std::string result;
	result.resize(12);
	put_hex(&result[0], mBytes[10]);
	put_hex(&result[2], mBytes[11]);
	put_hex(&result[4], mBytes[12]);
	put_hex(&result[6], mBytes[13]);
	put_hex(&result[8], mBytes[14]);
	put_hex(&result[10], mBytes[15]);
	return result;
}

bool uuid::parse(const std::string_view& pStr)
{
	if (pStr.length() != 36)
		return false;

	mBytes[0] = from_hex(&pStr[0]);
	mBytes[1] = from_hex(&pStr[2]);
	mBytes[2] = from_hex(&pStr[4]);
	mBytes[3] = from_hex(&pStr[6]);

	mBytes[4] = from_hex(&pStr[9]);
	mBytes[5] = from_hex(&pStr[11]);

	mBytes[6] = from_hex(&pStr[14]);
	mBytes[7] = from_hex(&pStr[16]);

	mBytes[8] = from_hex(&pStr[19]);
	mBytes[9] = from_hex(&pStr[21]);

	mBytes[10] = from_hex(&pStr[24]);
	mBytes[11] = from_hex(&pStr[26]);
	mBytes[12] = from_hex(&pStr[28]);
	mBytes[13] = from_hex(&pStr[30]);
	mBytes[14] = from_hex(&pStr[32]);
	mBytes[15] = from_hex(&pStr[34]);

	return true;
}

json uuid::to_json() const
{
	return to_string();
}

void uuid::from_json(const json& pJson)
{
	if (pJson.is_string() && pJson.get_ref<const json::string_t&>().length() == 36)
		parse(pJson);
	else if (pJson.is_array() && pJson.size() == 16)
		mBytes = pJson;
	else
	{
		log::error() << "Could not parse json uuid" << log::endm;
	}
}

hash::hash32_t uuid::to_hash32() const noexcept
{
	return hash::hash32(&mBytes[0], 16);
}

bool uuid::is_valid() const noexcept
{
	for (auto i : mBytes)
		if (i != 0)
			return true;
	return false;
}

uuid generate_uuid()
{
	std::random_device device;
	std::mt19937 generator{ device() };
	std::uniform_int_distribution<std::uint32_t> distribution{ 0, 255 };
	std::array<std::uint8_t, 16> bytes;
	for (auto& i : bytes)
		i = distribution(generator);

	// Set to version 4
	bytes[6] &= 0x0f;
	bytes[6] |= 0x40;

	// Varient 2 (?)
	bytes[8] &= 0xE0;
	bytes[8] |= 0x60;

	return{ bytes.begin(), bytes.end() };
}

} // namespace wge::util
