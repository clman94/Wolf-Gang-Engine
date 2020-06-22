
#include <wge/core/layer.hpp>

namespace wge::core
{

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
	assert(!mComponent_manager.get_storage<object_info>().has(id));
	mComponent_manager.add_component(id, object_info{});
	mComponent_manager.add_component(id, object_id_owner{ id, get_global_generator() });
	return get_object(id);
}

object layer::add_object(const std::string& pName)
{
	object obj = add_object();
	obj.set_name(pName);
	return obj;
}

void layer::remove_object(const object_id& pObject_id)
{
	mComponent_manager.remove_object(pObject_id);
}

void layer::remove_object(queue_destruction_flag, const object_id& pObject_id)
{
	mDestruction_queue.push_object(pObject_id);
}

void layer::remove_all_objects()
{
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

void layer::clear()
{
	mTime_scale = 1;
	mName.clear();
	remove_all_objects();
}

} // namespace wge::core
