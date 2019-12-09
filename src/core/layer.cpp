
#include <wge/core/layer.hpp>

namespace wge::core
{

layer::layer(const factory& pFactory) noexcept :
	mFactory(&pFactory)
{}

system* layer::get_system(int pID) const
{
	for (auto& i : mSystems)
		if (i->get_system_id() == pID)
			return i.get();
	return nullptr;
}

system* layer::get_system(const std::string& pName) const
{
	for (auto& i : mSystems)
		if (i->get_system_name() == pName)
			return i.get();
	return nullptr;
}

system* layer::add_system(int pType)
{
	// This system already exists.
	if (system* sys = get_system(pType))
		return sys;

	assert(mFactory);

	// Create a new one, otherwise.
	if (system::uptr sys = mFactory->create_system(pType, *this))
	{
		return mSystems.emplace_back(std::move(sys)).get();
	}
	return nullptr;
}

void layer::set_name(const std::string& pName) noexcept
{
	mName = pName;
}

const std::string& layer::get_name() const noexcept
{
	return mName;
}

object layer::add_object()
{
	object_id id = get_global_generator().get();
	if (mComponent_manager.get_storage<object_info>().has_component(id))
		log::warning("Object with id {} already exists.", id);
	mComponent_manager.add_component<object_info>(id);
	return get_object(id);
}

object layer::add_object(const std::string& pName)
{
	object_id id = get_global_generator().get();
	auto& info = mComponent_manager.add_component<object_info>(id);
	info.name = pName;
	return get_object(id);
}

void layer::remove_object(const object_id& pObject_id)
{
	get_global_generator().reclaim(pObject_id);
	mComponent_manager.remove_object(pObject_id);
}

void layer::remove_object(const object_id& pObject_id, queue_destruction_flag)
{
	mDestruction_queue.push_object(pObject_id);
}

void layer::remove_all_objects()
{
	for (auto i : *this)
		get_global_generator().reclaim(i.get_id());
	mComponent_manager.clear();
}

std::size_t layer::get_object_count() const noexcept
{
	return get_storage<object_info>().size();
}

void layer::set_enabled(bool pEnabled) noexcept
{
	mRecieve_update = pEnabled;
}

bool layer::is_enabled() const noexcept
{
	return mRecieve_update;
}

void layer::set_time_scale(float pScale) noexcept
{
	mTime_scale = pScale;
}

float layer::get_time_scale() const noexcept
{
	return mTime_scale;
}

void layer::preupdate(float pDelta)
{
	pDelta *= mTime_scale;
	for (auto& i : mSystems)
		i->preupdate(pDelta);
}

void layer::update(float pDelta)
{
	pDelta *= mTime_scale;
	for (auto& i : mSystems)
		i->update(pDelta);
}

void layer::postupdate(float pDelta)
{
	pDelta *= mTime_scale;
	for (auto& i : mSystems)
		i->postupdate(pDelta);
}

void layer::clear()
{
	mTime_scale = 1;
	mName.clear();
	mSystems.clear();

	remove_all_objects();
}

} // namespace wge::core
