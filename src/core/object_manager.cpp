#include <wge/core/object_manager.hpp>

namespace wge::core
{

object_data::object_data(object_id pId) :
	id(pId)
{
}


object_data & object_manager::add_object(object_id pId)
{
	return mObjects.emplace_back(pId);
}

void object_manager::remove_object(object_id pId)
{
	for (std::size_t i = 0; i < mObjects.size(); i++)
	{
		if (mObjects[i].id == pId)
		{
			mObjects.erase(mObjects.begin() + i);
			break;
		}
	}
}

std::size_t object_manager::get_object_count() const
{
	return mObjects.size();
}

// Registers a component for an object

void object_manager::register_component(component * pComponent)
{
	auto data = get_object_data(pComponent->get_object_id());
	WGE_ASSERT(data);
	data->components.push_back(pComponent);
}

void object_manager::unregister_component(component * pComponent)
{
	auto data = get_object_data(pComponent->get_object_id());
	WGE_ASSERT(data);
	for (std::size_t i = 0; i < data->components.size(); i++)
	{
		if (data->components[i] == pComponent)
		{
			data->components.erase(data->components.begin() + i);
			break;
		}
	}
}

object_data * object_manager::get_object_data(object_id pId)
{
	for (auto& i : mObjects)
		if (i.id == pId)
			return &i;
	return nullptr;
}

object_data * object_manager::get_object_data(std::size_t pIndex)
{
	return &mObjects[pIndex];
}

} // namespace wge::core
