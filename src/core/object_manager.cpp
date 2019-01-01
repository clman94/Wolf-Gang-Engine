#include <wge/core/object_manager.hpp>

namespace wge::core
{

object_data::object_data(object_id pId) :
	id(pId),
	name("New Object")
{
}

object_data& object_manager::add_object(object_id pId)
{
	return mObjects.emplace_back(pId);
}

void object_manager::remove_object(object_id pId)
{
	for (auto i = mObjects.begin(); i != mObjects.end(); i++)
	{
		if (i->id == pId)
		{
			mObjects.erase(i);
			break;
		}
	}
}

std::size_t object_manager::get_object_count() const noexcept
{
	return mObjects.size();
}

void object_manager::register_component(component* pComponent)
{
	auto data = get_object_data(pComponent->get_object_id());
	WGE_ASSERT(data);
	data->components.push_back({ pComponent->get_component_id(), pComponent->get_instance_id() });
}

void object_manager::unregister_component(component* pComponent)
{
	unregister_component(pComponent->get_object_id(), pComponent->get_instance_id());
}

void object_manager::unregister_component(object_id pObj_id, component_id pComp_id)
{
	auto data = get_object_data(pObj_id);
	WGE_ASSERT(data);
	for (std::size_t i = 0; i < data->components.size(); i++)
	{
		if (data->components[i].id == pComp_id)
		{
			data->components.erase(data->components.begin() + i);
			break;
		}
	}
}

object_data* object_manager::get_object_data(object_id pId)
{
	for (auto& i : mObjects)
		if (i.id == pId)
			return &i;
	return nullptr;
}

object_data* object_manager::get_object_data(std::size_t pIndex)
{
	auto i = mObjects.begin();
	std::advance(i, pIndex);
	return &(*i);
}

} // namespace wge::core
