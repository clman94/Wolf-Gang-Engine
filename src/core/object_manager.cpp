#include <wge/core/object_manager.hpp>

namespace wge::core
{

object_data::object_data() :
	id(util::generate_uuid()),
	name("New Object"),
	mTracker(std::make_shared<bool>(true))
{}

object_data::~object_data()
{
	*mTracker = false;
}

void object_data::cleanup_unused_components()
{
	for (std::size_t i = 0; i < components.size(); i++)
		if (components[i]->is_unused() ||
			components[i]->get_object_id() != id)
			components.erase(components.begin() + i--);
}

object_data& object_manager::add_object()
{
	return mObjects.emplace_back();
}

void object_manager::remove_object(const util::uuid& pId)
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

bool object_manager::has_object(const util::uuid& pId) const noexcept
{
	for (auto& i : mObjects)
		if (i.id == pId)
			return true;
	return false;
}

void object_manager::register_component(component* pComponent)
{
	auto data = get_object_data(pComponent->get_object_id());
	WGE_ASSERT(data);
	data->components.push_back(pComponent);
}

void object_manager::unregister_component(component* pComponent)
{
	unregister_component(pComponent->get_object_id(), pComponent->get_instance_id());
}

void object_manager::unregister_component(const util::uuid& pObj_id, const util::uuid& pComp_id)
{
	auto data = get_object_data(pObj_id);
	WGE_ASSERT(data);
	for (std::size_t i = 0; i < data->components.size(); i++)
	{
		if (data->components[i]->get_instance_id() == pComp_id)
		{
			data->components.erase(data->components.begin() + i);
			break;
		}
	}
}

object_data* object_manager::get_object_data(const util::uuid& pId)
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

void object_manager::clear()
{
	mObjects.clear();
}

} // namespace wge::core
