#include <catch2/catch.hpp>

#include <wge/filesystem/file_structure.hpp>

using namespace wge::filesystem;

using file_structure_test = file_structure<int>;

TEST_CASE("Files can be added or removed", "[file_structure]")
{
	file_structure_test fs;
	WHEN("An entry is inserted")
	{
		REQUIRE(fs.insert("file1", 5) != file_structure_test::iterator{});
		THEN("There is one more file at root")
		{
			REQUIRE(fs.exists("file1"));
			REQUIRE(fs.get_file_count({}) == 1);
		}
	}

	WHEN("An entry is removed")
	{
		REQUIRE(fs.remove("file1"));
		THEN("There are no files at root")
		{
			REQUIRE(!fs.exists("file1"));
			REQUIRE(fs.get_file_count({}) == 0);
		}
	}

	WHEN("An entry is created in a directory that doesn't exist")
	{
		REQUIRE(fs.insert("dir1/file1", 2) != file_structure_test::iterator{});
		THEN("That directory entry is automatically created")
		{
			REQUIRE(fs.exists("dir1"));
			REQUIRE(fs.get_file_count({}) == 1);
		}
		AND_THEN("The file is also in that directory")
		{
			REQUIRE(fs.exists("dir1/file1"));
			REQUIRE(fs.get_file_count("dir1") == 1);
		}
	}

	WHEN("An entry in a subdirectory is removed")
	{
		THEN("Only that file is removed")
		{
			REQUIRE(fs.exists("dir1"));
			REQUIRE(fs.get_file_count({}) == 1);
			REQUIRE(!fs.exists("dir1/file1"));
			REQUIRE(fs.get_file_count("dir1") == 0);
		}
		AND_THEN("The directory still exists")
		{
			REQUIRE(fs.exists("dir1"));
			REQUIRE(fs.get_file_count({}) == 1);
		}
	}

	WHEN("A directory with files is removed")
	{
		REQUIRE(fs.insert("dir1/file1", 1) != file_structure_test::iterator{});
		REQUIRE(fs.insert("dir1/file2", 2) != file_structure_test::iterator{});
		REQUIRE(fs.remove("dir1") == true);
		THEN("The directory, along with its files, are removed")
		{
			REQUIRE(!fs.exists("dir1"));
			REQUIRE(!fs.exists("dir1/fike1"));
			REQUIRE(!fs.exists("dir1/file2"));
			REQUIRE(fs.get_file_count({}) == 0);
		}
	}
}

TEST_CASE("Files can be iterated", "[file_structure]")
{
	GIVEN("A file structure with 2 files at root and a directory with a single file")
	{
		file_structure_test fs;
		REQUIRE(fs.insert("file1", 1) != file_structure_test::iterator{});
		REQUIRE(fs.exists("file1"));

		REQUIRE(fs.insert("file2", 2) != file_structure_test::iterator{});
		REQUIRE(fs.exists("file2"));

		REQUIRE(fs.insert("dir1/file3", 3) != file_structure_test::iterator{});
		REQUIRE(fs.exists("dir1/file3"));

		SECTION("Iterating the root with the directory iterator")
		{
			std::size_t count = 0;
			for (auto& i : fs.find({}).child())
				++count;
			REQUIRE(count == 3);
		}

		SECTION("Iterating directory \"dir1\" with the directory iterator")
		{
			std::size_t count = 0;
			for (auto& i : fs.find("dir1").child())
				++count;
			REQUIRE(count == 1);
		}
	}
}
