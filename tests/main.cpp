#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <wge/core/object_id.hpp>
#include <wge/util/ptr_adaptor.hpp>
#include <wge/core/component_manager.hpp>
#include <wge/core/destruction_queue.hpp>
#include <wge/util/ipair.hpp>
#include <wge/core/layer.hpp>
#include <wge/graphics/render_batch_2d.hpp>
#include <wge/util/ipair.hpp>
#include <wge/util/span.hpp>
#include <wge/math/vector.hpp>
#include <wge/util/ptr.hpp>
#include <wge/core/scene_resource.hpp>

using namespace wge;

struct tracker
{
	tracker() noexcept = default;
	tracker(const tracker& pOther) noexcept :
		copies(pOther.copies + 1)
	{}
	tracker(tracker&& pOther) noexcept :
		moves(pOther.moves + 1)
	{}
	std::size_t copies = 0;
	std::size_t moves = 0;
};

TEST_CASE("Copyable ptr")
{
	using util::copyable_ptr;

	copyable_ptr ptr = util::make_copyable_ptr<tracker>();
	REQUIRE(ptr.valid());
	REQUIRE(ptr->copies == 0);

	copyable_ptr copied_ptr = ptr;
	REQUIRE(copied_ptr.valid());
	REQUIRE(ptr->copies == 0);
	REQUIRE(copied_ptr->copies == 1);

	copyable_ptr moved_ptr = std::move(ptr);
	REQUIRE_FALSE(ptr.valid());
	REQUIRE(moved_ptr.valid());
	REQUIRE(moved_ptr->copies == 0);

	struct a { int x = 1;  };
	struct b : a { b() : a{ 2 } {} };
	struct d : a {};

	copyable_ptr poly_ptr = util::make_copyable_ptr<b, a>();
	REQUIRE(poly_ptr.valid());
	REQUIRE(poly_ptr->x == 2);
	copyable_ptr poly_ptr_copy = poly_ptr;
	REQUIRE(poly_ptr_copy->x == 2);
}

TEST_CASE("Layer can be copied")
{
	using core::layer;
	core::object_id id{ 1 };
	layer first;
	*first.add_component<int>(id) = 2;

	layer second = first;
	REQUIRE(*second.get_component<int>(id) == 2);
	REQUIRE(first.get_component<int>(id) != second.get_component<int>(id));
}

TEST_CASE("component_manager can be copied")
{
	using mgr = core::component_manager;
	core::object_id id{ 1 };
	mgr first;
	first.add_component<int>(id);
	*first.get_component<int>(id) = 13;
	REQUIRE_FALSE(first.get_storage<int>().empty());
	
	// Can copy it
	mgr second = first;
	REQUIRE(*second.get_component<int>(id) == 13);
	REQUIRE(first.get_component<int>(id) != second.get_component<int>(id));
}

TEST_CASE("A family represents a type")
{
	REQUIRE(core::family::from<int>() == core::family::from<int>());
	REQUIRE(core::family::from<int>() == core::family::from<int&>());
	REQUIRE(core::family::from<int>() == core::family::from<int&&>());
	REQUIRE(core::family::from<int>() == core::family::from<const int>());
	REQUIRE(core::family::from<int>() == core::family::from<const int&>());
	REQUIRE(core::family::from<int>() == core::family::from<const int&&>());
}

TEST_CASE("Vector")
{
	using namespace math;
	REQUIRE(fvec2{ 0, 0 }.is_zero());
	REQUIRE(fvec2{ 0.5f, 0.2f }.floor().is_zero());
	REQUIRE(fvec2{ 0.5f, 0.2f }.ceil() == fvec2{1, 1});
	REQUIRE(fvec2{ -1, -1 }.abs() == fvec2{ 1, 1 });
	REQUIRE(ivec2{ 1, 1 } + ivec2{ 1, 1 } == ivec2{ 2, 2 });
	REQUIRE(ivec2{ 1, 1 } - ivec2{ 1, 1 } == ivec2{ 0, 0 });
	REQUIRE(ivec2{ 2, 2 } * ivec2{ 2, 2 } == ivec2{ 4, 4 });
	REQUIRE(ivec2{ 2, 2 } / ivec2{ 2, 2 } == ivec2{ 1, 1 });
	REQUIRE((ivec2{ 1, 1 } += ivec2{ 1, 1 }) == ivec2{ 2, 2 });
	REQUIRE((ivec2{ 1, 1 } -= ivec2{ 1, 1 }) == ivec2{ 0, 0 });
	REQUIRE((ivec2{ 2, 2 } *= ivec2{ 2, 2 }) == ivec2{ 4, 4 });
	REQUIRE((ivec2{ 2, 2 } /= ivec2{ 2, 2 }) == ivec2{ 1, 1 });
	REQUIRE(ivec2{ fvec2{ 1.5f, 2.7f } } == ivec2{ 1, 2 });
	REQUIRE(ivec2{ 1, 2 }.components()[0] == 1);
	REQUIRE(ivec2{ 1, 2 }.components()[1] == 2);
	REQUIRE(ivec2{ 1, 2 }.swizzle(_x, _x) == ivec2{ 1, 1 });
	REQUIRE(ivec2{ 1, 2 }.swizzle(_y, _y) == ivec2{ 2, 2 });
	REQUIRE(ivec2{ 1, 2 }.swizzle(_y, _x) == ivec2{ 2, 1 });
}

TEST_CASE("Span")
{
	using util::span;
	span<int> default_span;
	REQUIRE(default_span.empty());
	REQUIRE(default_span.size() == 0);

	span<const int> const_span{ default_span };
	REQUIRE(const_span.empty());
	REQUIRE(const_span.size() == 0);

	std::array arr = { 'a', 'b', 'c' };
	auto arr_span = util::span(arr);
	REQUIRE_FALSE(arr_span.empty());
	REQUIRE(arr_span.size() == arr.size());

	span<const char> const_span2{ arr_span };
	REQUIRE_FALSE(const_span2.empty());
	REQUIRE(const_span2.size() == arr.size());

	const int arr2[] = { 1, 2, 3 };
	auto arr_span2 = util::span(arr2);
	REQUIRE_FALSE(arr_span2.empty());
	REQUIRE(arr_span2.size() == arr.size());
}

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

	REQUIRE(mgr.get_storage<com1>().has(id));
	REQUIRE(mgr.get_storage<com2>().has(id));
	REQUIRE(mgr.get_storage<com3>().has(id));

	queue.push_component(id, core::component_type::from<com1>());
	queue.push_component(id, core::component_type::from<com3>());

	queue.apply(mgr);
	// It should only remove the selected components.
	REQUIRE_FALSE(mgr.get_storage<com1>().has(id));
	REQUIRE_FALSE(mgr.get_storage<com3>().has(id));
	REQUIRE(mgr.get_storage<com2>().has(id));
	REQUIRE(queue.empty());

	queue.push_object(id);
	queue.apply(mgr);
	// The rest of the components should be removed from the object.
	REQUIRE_FALSE(mgr.get_storage<com2>().has(id));
	REQUIRE(queue.empty());

	// Clearing the queue should leave it empty.
	queue.push_component(id, core::component_type::from<com1>());
	queue.push_object(id);
	queue.clear();
	REQUIRE(queue.empty());
}

TEST_CASE("ipair{} creates pears")
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

