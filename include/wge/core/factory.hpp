#pragma once

#include <wge/core/component_manager.hpp>
#include <wge/core/component_type.hpp>
#include <wge/core/system.hpp>

#include <map>

namespace wge::core
{

class system;
class component;
class layer;

// The factory is the center hub for dependency injection for our
// components and systems.
// It also allows us to generate components and systems generically.
class factory
{
public:
	template <typename T>
	void register_component()
	{
		mComponent_factories[T::COMPONENT_ID] =
			[](component_manager& pManager) -> component*
		{
			return &pManager.add_component<T>();
		};
	}

	template <typename T, typename...Targs>
	void register_system(Targs&&...pExtra_args)
	{
		mSystem_factories[T::SYSTEM_ID] =
			// This will capture rvalues as values and lvalues as references.
			[pExtra_args = std::tuple<Targs...>(std::forward<Targs>(pExtra_args)...)](layer& pLayer)
			->system::uptr
		{
			auto args = std::tuple_cat(std::tie(pLayer), pExtra_args);
			auto make_unique_wrapper = [](auto&...pArgs) { return std::make_unique<T>(pArgs...); };
			return std::apply(make_unique_wrapper, args);
		};
	}

	component* create_component(const component_type& pType, component_manager& pManager) const;
	system::uptr create_system(int pType, layer& pLayer) const;

private:
	using component_factory = std::function<component*(component_manager&)>;
	using system_factory = std::function<system::uptr(layer&)>;
	std::map<component_type, component_factory> mComponent_factories;
	std::map<int, system_factory> mSystem_factories;
};

} // namespace wge::core
