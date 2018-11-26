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
	auto& data = mObjects.emplace_back(get_context().get_unique_instance_id());
	return{ *this, data.id };
}

void layer::remove_object(const game_object& mObj)
{
	for (std::size_t i = 0; i < mObjects.size(); i++)
	{
		if (mObjects[i].id == mObj.get_instance_id())
		{
			mComponent_manager.remove_entity(mObj.get_instance_id());
			mObjects.erase(mObjects.begin() + i);
			return;
		}
	}
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

layer::object_data::object_data(instance_id pId) :
	id(pId),
	name("New Object")
{}

} // namespace wge::core
