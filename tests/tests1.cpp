#ifdef TESTS
#define CATCH_CONFIG_MAIN

#include "catch/single_include/catch.hpp"

#include <engine/renderer.hpp>

engine::renderer::key_code key_name_to_code(const std::string& pName);
std::string key_code_to_name(engine::renderer::key_code pCode);

namespace {

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
	std::getchar();
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
		REQUIRE(key_code_to_name(engine::renderer::key_code::Z) == "Z");
	}
	SECTION("invalid input")
	{
		REQUIRE(key_code_to_name(engine::renderer::key_code::Unknown) == std::string());
	}
	std::getchar();
}


}


#endif