#include <wge/util/uuid.hpp>

#include <random>

namespace wge::util
{

static void put_hex(char* pDest, std::uint8_t pVal)
{
	constexpr const char* chars = "0123456789abcdef";
	pDest[0] = chars[pVal >> 4];
	pDest[1] = chars[pVal & 0x0f];
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

int uuid::compare(const uuid & pR) const noexcept
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
	result[22] = '-';
	put_hex(&result[23], mBytes[10]);
	put_hex(&result[25], mBytes[11]);
	put_hex(&result[27], mBytes[12]);
	put_hex(&result[29], mBytes[13]);
	put_hex(&result[31], mBytes[14]);
	put_hex(&result[33], mBytes[15]);

	return result;
}

json uuid::to_json() const
{
	return mBytes;
}

void uuid::from_json(const json & pJson)
{
	mBytes = pJson;
}

hash::hash32_t uuid::to_hash32() const noexcept
{
	return hash::hash32(&mBytes[0], 16);
}

uuid generate_uuid()
{
	std::random_device device;
	std::seed_seq seed{ device(), device(), device(), device(), device() };
	std::mt19937 generator{ seed };
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
