#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <wge/core/object_id.hpp>
#include <wge/util/ptr_adaptor.hpp>
#include <wge/core/component_manager.hpp>
#include <wge/core/destruction_queue.hpp>
#include <wge/util/ipair.hpp>

using namespace wge;

TEST_CASE("Object id generator can generate and reuse ids")
{
	using core::object_id;
	core::object_id_generator gen;
	for (int i = 1; i < 100; i++)
		REQUIRE(gen.get() == i);

	for (int i = 1; i < 100; i++)
		gen.reclaim(i);

	// Values above the internal counter should not do anything.
	for (int i = 2; i < 100; i++)
	{
		gen.reclaim(i);

		// Counter isn't affected so it should always return 1.
		// Had a run-in with this once... wasn't pretty.
		object_id id = gen.get();
		REQUIRE(id == object_id(1));
		gen.reclaim(id);
	}

	// We can reuse all the ids starting from 1.
	for (int i = 1; i < 100; i++)
		REQUIRE(gen.get() == i);
	
	// get() should cleans out duplicates properly.
	gen.reclaim(2);
	gen.reclaim(2);
	gen.reclaim(3);
	gen.reclaim(3);
	REQUIRE(gen.get() == 3);
	REQUIRE(gen.get() == 2);
	// The last get should be from the counter.
	REQUIRE(gen.get() == 100);

	// reclaim() should clean out duplicates properly.
	core::object_id_generator gen2;
	for (int i = 0; i < 10; i++)
		gen2.get();
	// Reclaim twice for each value.
	for (int i = 1; i <= 9; i++)
	{
		gen2.reclaim(i);
		gen2.reclaim(i);
	}
	// This last one should unwind the counter and
	// remove duplicates when it comes across them.
	gen2.reclaim(10);
	// Counter should be unwound to 1 again.
	for (int i = 1; i < 10; i++)
		REQUIRE(gen2.get() == i);
}

TEST_CASE("ptr_adaptor adapts nicely")
{
	using util::ptr_adaptor;

	struct counted
	{
		counted() = default;
		counted(const counted& pR) :
			counter(pR.counter + 1)
		{}

		int counter = 0;
	};

	// Copies rvalues.
	ptr_adaptor copy_value = counted{};
	REQUIRE(copy_value->counter == 1);
	ptr_adaptor copy_value2 = copy_value;
	REQUIRE(copy_value2->counter == 2);

	// References lvalues.
	int lvalue = 0;
	ptr_adaptor ref = lvalue;
	REQUIRE(*ref == 0);
	lvalue = 1;
	REQUIRE(*ref == 1);

	// Wraps pointers
	int pointed = 0;
	ptr_adaptor ptr = &pointed;
	REQUIRE(*ptr == 0);
	pointed = 1;
	REQUIRE(*ptr == 1);
}

TEST_CASE("destruction_queue works")
{
	core::component_manager mgr;
	core::destruction_queue queue;

	// A set of components
	struct com1 { };
	struct com2 { };
	struct com3 { };

	core::object_id id(1);

	mgr.add_component<com1>(id);
	mgr.add_component<com2>(id);
	mgr.add_component<com3>(id);

	REQUIRE(mgr.get_storage<com1>().has_component(id));
	REQUIRE(mgr.get_storage<com2>().has_component(id));
	REQUIRE(mgr.get_storage<com3>().has_component(id));

	queue.push_component(id, core::component_type::from<com1>());
	queue.push_component(id, core::component_type::from<com3>());

	queue.apply(mgr);
	// It should only remove the selected components.
	REQUIRE_FALSE(mgr.get_storage<com1>().has_component(id));
	REQUIRE_FALSE(mgr.get_storage<com3>().has_component(id));
	REQUIRE(mgr.get_storage<com2>().has_component(id));
	REQUIRE(queue.empty());

	queue.push_object(id);
	queue.apply(mgr);
	// The rest of the components should be removed from the object.
	REQUIRE_FALSE(mgr.get_storage<com2>().has_component(id));
	REQUIRE(queue.empty());

	// Clearing the queue should leave it empty.
	queue.push_component(id, core::component_type::from<com1>());
	queue.push_object(id);
	queue.clear();
	REQUIRE(queue.empty());
}

TEST_CASE("ipair creates pears")
{
	std::array<int, 100> arr;
	// v is a reference to the original array item.
	for (auto [i, v] : util::ipair{ arr })
		v = i;

	// Values should have been changed.
	for (auto [i, v] : util::ipair{ arr })
		REQUIRE(i == v);

	// All values should be equal to their index.
	for (std::size_t i = 0; i < arr.size(); i++)
		REQUIRE(arr[i] == i);
}
