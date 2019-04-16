
#include <wge/core/game_object.hpp>
#include <wge/core/component.hpp>
#include <wge/core/context.hpp>
#include <wge/core/layer.hpp>

namespace wge::core
{

game_object::game_object() noexcept :
	mLayer(nullptr),
	mData(nullptr)
{
}

game_object::game_object(layer& pLayer, object_data& pData) noexcept :
	mLayer(&pLayer),
	mData(&pData)
{
}

bool game_object::has_component(const component_type& pType) const
{
	assert_valid_reference();
	for (auto& i : mData->components)
		if (i->get_component_id() == pType)
			return true;
	return false;
}

json game_object::serialize(serialize_type pType) const
{
	assert_valid_reference();
	json result;
	result["name"] = get_name();
	result["id"] = get_instance_id();
	for (auto& i : mData->components)
		result["components"].push_back(i->serialize(pType));
	return result;
}

void game_object::deserialize(const json& pJson)
{
	assert_valid_reference();
	//set_instance_id(pJson["id"]);
	set_name(pJson["name"]);
	for (auto& i : pJson["components"])
	{
		component* c = add_component(i["type"]);
		if (!c)
			continue;
		c->deserialize(*this, i);
	}
}

component* game_object::add_component(const component_type& pType)
{
	assert_valid_reference();
	return get_layer().add_component(*this, pType);
}

std::size_t game_object::get_component_count() const
{
	assert_valid_reference();
	mData->cleanup_unused_components();
	return mData->components.size();
}

component* game_object::get_component_at(std::size_t pIndex)
{
	assert_valid_reference();
	mData->cleanup_unused_components();
	return mData->components[pIndex];
}

component* game_object::get_component(const std::string& pName)
{
	assert_valid_reference();
	mData->cleanup_unused_components();
	for (auto& i : mData->components)
		if (i->get_name() == pName)
			return i;
	return nullptr;
}

component* game_object::get_component_by_type(const std::string& pName)
{
	assert_valid_reference();
	mData->cleanup_unused_components();
	for (auto& i : mData->components)
		if (i->get_component_name() == pName)
			return i;
	return nullptr;
}

component* game_object::get_component(const component_type& pType) const
{
	assert_valid_reference();
	mData->cleanup_unused_components();
	for (auto& i : mData->components)
		if (i->get_component_id() == pType)
			return i;
	return nullptr;
}

void game_object::move_component(std::size_t pFrom, std::size_t pTo)
{
	assert_valid_reference();
	mData->cleanup_unused_components();
	if (pFrom == pTo)
		return;
	if (pFrom < pTo)
	{
		auto iter_begin = mData->components.begin() + pFrom;
		auto iter_end = mData->components.begin() + pTo + 1;
		std::rotate(iter_begin, iter_begin + 1, iter_end);
	}
	else
	{
		auto iter_begin = mData->components.rbegin() + pFrom;
		auto iter_end = mData->components.rbegin() + pTo + 1;
		std::rotate(iter_begin, iter_begin + 1, iter_end);
	}
}

void game_object::remove_component(std::size_t pIndex)
{
	assert_valid_reference();
	mData->cleanup_unused_components();
	mData->components[pIndex]->destroy();
}

void game_object::remove_all_components()
{
	assert_valid_reference();
	for (auto& i : mData->components)
		i->destroy();
	mData->components.clear();
}

const std::string& game_object::get_name() const
{
	assert_valid_reference();
	return mData->name;
}

void game_object::set_name(const std::string & pName)
{
	assert_valid_reference();
	mData->name = pName;
}

void game_object::destroy()
{
	assert_valid_reference();;
	get_layer().remove_object(*this);
	mData = nullptr;
}

layer& game_object::get_layer() const
{
	assert_valid_reference();
	return *mLayer;
}

context& game_object::get_context() const
{
	assert_valid_reference();
	return get_layer().get_context();
}

const util::uuid& game_object::get_instance_id() const
{
	assert_valid_reference();
	return mData->id;
}

bool game_object::operator==(const game_object& pObj) const noexcept
{
	return mData && pObj.mData && mData->id == pObj.mData->id;
}

void game_object::assert_valid_reference() const
{
	if (!is_valid())
		throw invalid_game_object{};
}

} // namespace wge::core
