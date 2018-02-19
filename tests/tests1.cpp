#ifdef TESTS
#define CATCH_CONFIG_RUNNER

#include "catch/single_include/catch.hpp"

#include <engine/renderer.hpp>
#include <engine/utility.hpp>
#include <engine/binary_util.hpp>

#include <sstream>

engine::renderer::key_code key_name_to_code(const std::string& pName);
std::string key_code_to_name(engine::renderer::key_code pCode);


int main(int argc, char* const argv[])
{
	int r = Catch::Session().run();
	std::getchar();
	return r;
}

namespace {

TEST_CASE("remove_trailing_whitespace")
{
	REQUIRE(util::remove_trailing_whitespace("  d a   ") == "d a");
	REQUIRE(util::remove_trailing_whitespace("\t\r\nd aa s djk \t\r\n\n\r\t") == "d aa s djk");
	REQUIRE(util::remove_trailing_whitespace("   \n\r\t  ") == std::string());
}

TEST_CASE("Binary read and write unsigned values")
{
	SECTION("Conversion Integrety uint16_t")
	{
		REQUIRE(sizeof(uint16_t) == 2);

		for (uint16_t orig = 0; orig < 1000; orig++)
		{
			std::stringstream stream;
			binary_util::write_unsignedint_binary(stream, orig);
			stream.seekg(0);
			uint16_t val = binary_util::read_unsignedint_binary<uint16_t>(stream);
			REQUIRE(orig == val);
		}
	}

	SECTION("Conversion Integrety uint32_t")
	{
		REQUIRE(sizeof(uint32_t) == 4);

		for (uint32_t orig = 0; orig < 1000; orig++)
		{
			std::stringstream stream;
			binary_util::write_unsignedint_binary(stream, orig);
			stream.seekg(0);
			uint32_t val = binary_util::read_unsignedint_binary<uint32_t>(stream);
			REQUIRE(orig == val);
		}
	}

	SECTION("Conversion Integrety uint64_t")
	{
		REQUIRE(sizeof(uint64_t) == 8);

		for (uint64_t orig = 0; orig < 1000; orig++)
		{
			std::stringstream stream;
			binary_util::write_unsignedint_binary(stream, orig);
			stream.seekg(0);
			uint64_t val = binary_util::read_unsignedint_binary<uint64_t>(stream);
			REQUIRE(orig == val);
		}
	}
}
TEST_CASE("key_name_to_code")
{
	SECTION("num0-num9")
	{
		REQUIRE(key_name_to_code("N U M0 ") == engine::renderer::key_code::Num0);
		REQUIRE(key_name_to_code("nU m1") == engine::renderer::key_code::Num1);
		REQUIRE(key_name_to_code("NU  m2") == engine::renderer::key_code::Num2);
		REQUIRE(key_name_to_code("num3  ") == engine::renderer::key_code::Num3);
		REQUIRE(key_name_to_code("num4") == engine::renderer::key_code::Num4);
		REQUIRE(key_name_to_code("num 9") == engine::renderer::key_code::Num9);
	}
	SECTION("a-z")
	{
		REQUIRE(key_name_to_code("A ") == engine::renderer::key_code::A);
		REQUIRE(key_name_to_code("b") == engine::renderer::key_code::B);
		REQUIRE(key_name_to_code(" C") == engine::renderer::key_code::C);
		REQUIRE(key_name_to_code("Z ") == engine::renderer::key_code::Z);
	}
	SECTION("invalid input")
	{
		REQUIRE(key_name_to_code("artj") == engine::renderer::key_code::Unknown);
		REQUIRE(key_name_to_code("aea") == engine::renderer::key_code::Unknown);
		REQUIRE(key_name_to_code("jtr4") == engine::renderer::key_code::Unknown);
		REQUIRE(key_name_to_code("7537a") == engine::renderer::key_code::Unknown);
	}
}

TEST_CASE("key_code_to_name")
{
	SECTION("num0-num9")
	{
		REQUIRE(key_code_to_name(engine::renderer::key_code::Num0) == "num0");
		REQUIRE(key_code_to_name(engine::renderer::key_code::Num1) == "num1");
		REQUIRE(key_code_to_name(engine::renderer::key_code::Num2) == "num2");
		REQUIRE(key_code_to_name(engine::renderer::key_code::Num3) == "num3");
		REQUIRE(key_code_to_name(engine::renderer::key_code::Num9) == "num9");
	}
	SECTION("a-z")
	{
		REQUIRE(key_code_to_name(engine::renderer::key_code::A) == "a");
		REQUIRE(key_code_to_name(engine::renderer::key_code::B) == "b");
		REQUIRE(key_code_to_name(engine::renderer::key_code::C) == "c");
		REQUIRE(key_code_to_name(engine::renderer::key_code::D) == "d");
		REQUIRE(key_code_to_name(engine::renderer::key_code::Z) == "z");
	}
	SECTION("invalid input")
	{
		REQUIRE(key_code_to_name(engine::renderer::key_code::Unknown) == std::string());
	}
}


}


#endif