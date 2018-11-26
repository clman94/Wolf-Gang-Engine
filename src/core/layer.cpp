#include <wge/core/layer.hpp>
#include <wge/core/context.hpp>

namespace wge::core
{

game_object layer::create_object()
{
	auto& data = mObjects.emplace_back(object_data{ "New Object", get_context().get_unique_instance_id() });
	return{ *this, data.id };
}

game_object layer::get_object(std::size_t pIndex)
{
	if (pIndex >= mObjects.size())
		return{ *this };
	return{ *this, mObjects[pIndex].id };
}

game_object layer::get_object(instance_id pId)
{
	for (auto& i : mObjects)
		if (i.id == pId)
			return{ *this, i.id };
	return{ *this };
}

std::size_t layer::get_object_count() const
{
	return mObjects.size();
}

std::string layer::get_object_name(const game_object & mObj)
{
	for (auto& i : mObjects)
		if (i.id == mObj.get_instance_id())
			return i.name;
	return{};
}

} // namespace wge::core
