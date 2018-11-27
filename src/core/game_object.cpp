
#include <wge/core/game_object.hpp>
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

game_object::game_object(layer& pLayer) :
	mLayer(pLayer),
	mData(nullptr)
{
}

game_object::game_object(layer& pLayer, object_data& pData) :
	mLayer(pLayer),
	mData(&pData)
{
}

bool game_object::has_component(int pId) const
{
	WGE_ASSERT(mData);
	for (auto& i : mData->components)
		if (i.type == pId)
			return true;
	return false;
}

std::size_t game_object::get_component_count() const
{
	WGE_ASSERT(mData);
	return mData->components.size();
}

component* game_object::get_component_index(std::size_t pIndex)
{
	WGE_ASSERT(mData);
	auto& comp_entry = mData->components[pIndex];
	return get_layer().get_component(comp_entry.type, comp_entry.id);
}

component* game_object::get_component(const std::string & pName)
{
	WGE_ASSERT(mData);
	for (auto& i : mData->components)
	{
		component* comp = get_layer().get_component(i.type, i.id);
		if (comp->get_name() == pName)
			return comp;
	}
	return nullptr;
}

component* game_object::get_component(int pType) const
{
	WGE_ASSERT(mData);
	return get_layer().get_first_component(*this, pType);
}

void game_object::remove_component(std::size_t pIndex)
{
	WGE_ASSERT(mData);
	auto& comp_entry = mData->components[pIndex];
	get_layer().remove_component(comp_entry.type, comp_entry.id);
}

void game_object::remove_components()
{
	WGE_ASSERT(mData);
}

const std::string& game_object::get_name() const
{
	WGE_ASSERT(mData);
	return mData->name;
}

void game_object::set_name(const std::string & pName)
{
	WGE_ASSERT(mData);
	mData->name = pName;
}

void game_object::destroy()
{
	WGE_ASSERT(mData);
	get_layer().remove_object(*this);
	mData = nullptr;
}

layer& game_object::get_layer() const
{
	WGE_ASSERT(mData);
	return mLayer;
}

object_id game_object::get_instance_id() const
{
	return mData->id;
}

void game_object::set_instance_id(object_id pId)
{
	mData->id = pId;
}

bool game_object::operator==(const game_object & pObj) const
{
	return mData && pObj.mData && mData->id == pObj.mData->id;
}

} // namespace wge::core
