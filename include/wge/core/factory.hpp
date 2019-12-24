#pragma once

#include <wge/core/component_manager.hpp>
#include <wge/core/component_type.hpp>
#include <wge/core/system.hpp>
#include <wge/util/ptr.hpp>

#include <map>

namespace wge::core
{

class system;
class component;
class layer;

// The factory is the center hub for dependency injection for our systems.
// It also allows us to generate components and systems generically.
class factory
{
public:
	template <typename T, typename...Targs>
	void register_system(Targs&&...pExtra_args)
	{
		mSystem_factories[T::SYSTEM_ID] =
			// This will capture rvalues as values and lvalues as references.
			[pExtra_args = std::tuple<Targs...>(std::forward<Targs>(pExtra_args)...)](layer& pLayer)
			->util::copyable_ptr<system>
		{
			auto make_unique_wrapper = [](auto&...pArgs) { return util::make_copyable_ptr<T, system>(pArgs...); };
			return std::apply(make_unique_wrapper, pExtra_args);
		};
	}

	util::copyable_ptr<system> create_system(int pType, layer& pLayer) const;

private:
	using system_factory = std::function<util::copyable_ptr<system>(layer&)>;
	std::map<int, system_factory> mSystem_factories;
};

} // namespace wge::core
