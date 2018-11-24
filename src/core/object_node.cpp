
#include <wge/core/object_node.hpp>
#include <wge/core/component.hpp>
#include <wge/core/context.hpp>
#include <wge/core/layer.hpp>

namespace wge::core
{

inline bool starts_with(const std::string& pStr, const std::string& pPrefix)
{
	if (pStr.length() < pPrefix.length())
		return false;
	return std::string_view(pStr).substr(0, pPrefix.length()) == pPrefix;
}

// Generates a uniquely numbered name from a list
template <typename Titer, typename Tgetter>
inline std::string create_unique_name(std::string pPrefix, Titer pBegin, Titer pEnd, Tgetter&& pGetter)
{
	// This is the largest value found
	int max = 0;

	// True if we found an exact match to pPrefix
	bool found_exact = false;

	const std::string prefix_numbered = pPrefix + "_";
	for (; pBegin != pEnd; pBegin++)
	{
		const std::string& str = pGetter(*pBegin);

		// This hasn't found any numbered matches yet,
		// but it found an exact match to the pPrefix.
		if (pPrefix == str && max == 0)
			found_exact = true;

		// Found a match to a numbered value
		else if (starts_with(str, prefix_numbered)
			&& str.length() > prefix_numbered.length())
		{
			// Parse number
			int val = std::atoi(str.substr(prefix_numbered.length()).c_str());

			// Record it if it is larger
			if (val > max)
				max = val;
		}
	}

	if (!found_exact)
		return pPrefix;
	else
		return pPrefix + "_" + std::to_string(max + 1);
}

object_node::object_node(layer& pLayer) :
	mLayer(pLayer),
	mInstance_id(pLayer)
{
}

object_node::~object_node()
{
}

bool object_node::has_component(int pId) const
{
	for (auto& i : get_layer().)
		if (i->get_component_id() == pId)
			return true;
	return false;
}

component* object_node::get_component(const std::string & pName)
{
	return nullptr;
}

component* object_node::get_component(int pId) const
{
	for (auto& i : mComponents)
		if (i->get_component_id() == pId)
			return i.get();
	return nullptr;
}

void object_node::remove_component(std::size_t pIndex)
{
	mComponents.erase(mComponents.begin() + pIndex);
}

void object_node::remove_components()
{
	mComponents.clear();
}

layer& object_node::get_layer() const
{
	return mContemLayerxt;
}

} // namespace wge::core
