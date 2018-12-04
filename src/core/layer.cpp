#include <wge/core/layer.hpp>
#include <wge/core/context.hpp>

namespace wge::core
{
layer::layer(context & pContext) :
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

void layer::set_name(const std::string_view& pName)
{
	mName = pName;
}

const std::string& layer::get_name() const
{
	return mName;
}

game_object layer::create_object()
{
	auto& data = mObject_manager.add_object(get_context().get_unique_instance_id());
	return{ *this, data };
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

std::size_t layer::get_object_count() const
{
	return mObject_manager.get_object_count();
}

context & layer::get_context() const
{
	return mContext;
}

void layer::set_enabled(bool pEnabled)
{
	mRecieve_update = pEnabled;
}

bool layer::is_enabled() const
{
	return mRecieve_update;
}

float layer::get_time_scale() const
{
	return mTime_scale;
}

void layer::set_time_scale(float pScale)
{
	mTime_scale = pScale;
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
