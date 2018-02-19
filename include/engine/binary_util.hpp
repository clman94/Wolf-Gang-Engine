namespace binary_util
{

// Read an unsigned integer value (of any size) from a stream.
// Format is little endian (for convenience)
template<typename T>
inline T read_unsignedint_binary(std::istream& pStream)
{
	if constexpr(!std::is_unsigned<T>())
		static_assert("T is not unsigned");

	uint8_t bytes[sizeof(T)];// char gives negative values and causes issues when converting to unsigned.
	if (!pStream.read((char*)&bytes, sizeof(T))) // And passing this unsigned char array as a char* works well.
		return 0;

	T val = 0;
	for (size_t i = 0; i < sizeof(T); i++)
		val += static_cast<T>(bytes[i]) << (i * 8);
	return val;
}

// Stores an unsigned integer value (of any size) to a stream.
// Format is little endian (for convenience)
template<typename T>
inline bool write_unsignedint_binary(std::ostream& pStream, const T pVal)
{
	if constexpr(!std::is_unsigned<T>())
		static_assert("T is not unsigned");

	uint8_t bytes[sizeof(T)];
	for (size_t i = 0; i < sizeof(T); i++)
		bytes[i] = static_cast<uint8_t>(static_cast<T>(pVal & (0xFF << (8 * i))) >> (8 * i));
	pStream.write((char*)&bytes, sizeof(T));
	return pStream.good();
}

}