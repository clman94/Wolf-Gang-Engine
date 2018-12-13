#include <wge/core/layer.hpp>
#include <wge/core/context.hpp>

namespace wge::core
{
layer::layer(context & pContext) noexcept :
	mContext(pContext)
{
}
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

void layer::set_name(const std::string_view& pName) noexcept
{
	mName = pName;
}

const std::string& layer::get_name() const noexcept
{
	return mName;
}

game_object layer::create_object()
{
	auto& data = mObject_manager.add_object(get_context().get_unique_instance_id());
	return{ *this, data };
}

game_object layer::create_object(const std::string & pName)
{
	game_object obj = create_object();
	obj.set_name(pName);
	return obj;
}

void layer::remove_object(const game_object& mObj)
{
	mComponent_manager.remove_object(mObj.get_instance_id());
	mObject_manager.remove_object(mObj.get_instance_id());
}

game_object layer::get_object(std::size_t pIndex)
{
	auto data = mObject_manager.get_object_data(pIndex);
	if (!data)
		return{ *this };
	return{ *this, *data };
}

game_object layer::get_object(object_id pId)
{
	auto data = mObject_manager.get_object_data(pId);
	if (data)
		return{ *this, *data };
	return{ *this };
}

std::size_t layer::get_object_count() const noexcept
{
	return mObject_manager.get_object_count();
}

component* layer::get_first_component(const game_object & pObj, int pType)
{
	return mComponent_manager.get_first_component(pType, pObj.get_instance_id());
}

component* layer::get_component(int pType, component_id pId)
{
	return mComponent_manager.get_component(pType, pId);
}

void layer::remove_component(int pType, component_id pId)
{
	component* comp = get_component(pType, pId);
	object_id obj_id = comp->get_object_id();
	mObject_manager.unregister_component(obj_id, pId);
	mComponent_manager.remove_component(pType, pId);
}

context & layer::get_context() const noexcept
{
	return mContext;
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

} // namespace wge::core
