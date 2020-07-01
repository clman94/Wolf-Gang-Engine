#include <wge/util/uuid.hpp>
#include <wge/logging/log.hpp>

#include <random>
#include <cctype>

namespace wge::util
{

static void put_hex(char* pDest, std::uint8_t pVal)
{
	constexpr const char* chars = "0123456789abcdef";
	pDest[0] = chars[pVal >> 4];
	pDest[1] = chars[pVal & 0x0f];
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
