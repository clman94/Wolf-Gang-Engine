
#include <wge/core/game_object.hpp>
#include <wge/core/component.hpp>
#include <wge/core/context.hpp>
#include <wge/core/layer.hpp>

namespace wge::core
{

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

json game_object::serialize(serialize_type pType) const
{
	json result;
	result["id"] = get_instance_id();
	for (const auto& i : mData->components)
	{
		component* comp = get_layer().get_component(i.type, i.id);
		result["components"].push_back(comp->serialize(pType));
	}
	return result;
}

std::size_t game_object::get_component_count() const
{
	WGE_ASSERT(mData);
	return mData->components.size();
}

component* game_object::get_component_at(std::size_t pIndex)
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

layer& game_object::get_layer() const noexcept
{
	WGE_ASSERT(mData);
	return mLayer;
}

context& game_object::get_context() const noexcept
{
	return get_layer().get_context();
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
