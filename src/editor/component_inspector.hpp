#pragma once

#include <functional>
#include <map>

#include <wge/math/vector.hpp>

namespace wge::core
{
class component;
}

namespace wge::editor
{

// This class stores a list of inspectors to be used for
// each type of component.
class component_inspector
{
public:
	// Assign an inspector for a component
	void add_inspector(int pComponent_id, std::function<void(core::component*)> pFunc)
	{
		mInspector_guis[pComponent_id] = pFunc;
	}

	// Show the inspector's gui for this component
	void on_gui(core::component* pComponent)
	{
		if (auto func = mInspector_guis[pComponent->get_component_id()])
			func(pComponent);
	}

private:
	std::map<int, std::function<void(core::component*)>> mInspector_guis;
};

} // namespace wge::editor
