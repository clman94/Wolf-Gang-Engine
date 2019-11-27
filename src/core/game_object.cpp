
#include <wge/core/game_object.hpp>
#include <wge/core/component.hpp>
#include <wge/core/scene.hpp>
#include <wge/core/layer.hpp>

namespace wge::core
{

object::object() noexcept :
	mLayer(nullptr)
{
}

object::object(layer& pLayer, const handle<object_info>& pInfo) noexcept :
	mLayer(&pLayer),
	mInfo(pInfo)
{}

bool object::has_component(const component_type& pType) const
{
	assert_valid_reference();
	auto& components = mInfo->components;
	return std::find(components.begin(), components.end(), pType) != components.end();
}

std::size_t object::get_component_count() const
{
	assert_valid_reference();
	return mInfo->components.size();
}

void object::move_component(std::size_t pFrom, std::size_t pTo)
{
	assert_valid_reference();
	if (pFrom == pTo)
		return;
	if (pFrom < pTo)
	{
		auto& components = mInfo->components;
		auto iter_begin = components.begin() + pFrom;
		auto iter_end = components.begin() + pTo + 1;
		std::rotate(iter_begin, iter_begin + 1, iter_end);
	}
	else
	{
		auto& components = mInfo->components;
		auto iter_begin = components.rbegin() + pFrom;
		auto iter_end = components.rbegin() + pTo + 1;
		std::rotate(iter_begin, iter_begin + 1, iter_end);
	}
}

const std::string& object::get_name() const
{
	assert_valid_reference();
	return mInfo->name;
}

void object::set_name(const std::string & pName)
{
	assert_valid_reference();
	mInfo->name = pName;
}

void object::destroy()
{
	assert_valid_reference();;
	get_layer().remove_object(get_id());
	reset();
}

void object::destroy(queue_destruction_flag)
{
	assert_valid_reference();
	get_layer().remove_object(get_id(), queue_destruction);
	reset();
}

layer& object::get_layer() const
{
	assert_valid_reference();
	return *mLayer;
}

bool object::is_instanced() const noexcept
{
	assert_valid_reference();
	return static_cast<bool>(mInfo->source_asset);
}

void object::set_asset(const core::asset::ptr& pAsset) noexcept
{
	assert_valid_reference();
	mInfo->source_asset = pAsset;
}

asset::ptr object::get_asset() const
{
	assert_valid_reference();
	return mInfo->source_asset;
}

object_id object::get_id() const
{
	assert_valid_reference();
	return mInfo.get_object_id();
}

bool object::operator==(const object& pObj) const noexcept
{
	return mInfo.is_valid() && pObj.mInfo.is_valid() && mInfo.get_object_id() == pObj.mInfo.get_object_id();
}

void object::assert_valid_reference() const
{
	if (!is_valid())
		throw invalid_game_object{};
}

} // namespace wge::core
