
#include <wge/core/engine.hpp>
#include <wge/core/layer.hpp>
#include <wge/core/context.hpp>
#include <wge/util/uuid_rerouter.hpp>

namespace wge::core
{

layer::layer(context& pContext) noexcept :
	mContext(pContext)
{}

json layer::serialize(serialize_type pType)
{
	json result;
	if (pType & serialize_type::properties)
	{
		result["name"] = mName;
		result["timescale"] = mTime_scale;
		result["enabled"] = mRecieve_update;
	}

	json& j_systems = result["systems"];
	for (auto& i : mSystems)
		j_systems.push_back(i->serialize(pType));

	json& j_objects = result["objects"];
	for (std::size_t i = 0; i < get_object_count(); i++)
		j_objects.push_back(get_object(i).serialize(pType));

	return result;
}

void layer::deserialize(const json& pJson)
{
	clear();

	util::uuid_rerouter rerouter;

	mName = pJson["name"];
	mTime_scale = pJson["timescale"];
	mRecieve_update = pJson["enabled"];

	const factory& factory = get_engine().get_factory();
	for (auto& i : pJson["systems"])
	{
		system* sys = add_system(i["type"]);
		sys->deserialize(i);
	}

	for (auto& i : pJson["objects"])
	{
		game_object obj = add_object();
		obj.deserialize(i);
	}
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

system* layer::add_system(int pType)
{
	// This system already exists.
	if (system* sys = get_system(pType))
		return sys;

	// Create a new one, otherwise.
	const factory& factory = get_engine().get_factory();
	if (system::uptr sys = factory.create_system(pType, *this))
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

game_object layer::add_object()
{
	auto& data = mObject_manager.add_object();
	return{ *this, data };
}

game_object layer::add_object(const std::string& pName)
{
	game_object obj = add_object();
	obj.set_name(pName);
	return obj;
}

void layer::remove_object(game_object& mObj)
{
	mObj.remove_all_components();
	mObject_manager.remove_object(mObj.get_instance_id());
}

void layer::remove_all_objects()
{
	mObject_manager.clear();
	mComponent_manager.clear();
}

game_object layer::get_object(std::size_t pIndex)
{
	object_data* data = mObject_manager.get_object_data(pIndex);
	if (!data)
		return{};
	return{ *this, *data };
}

game_object layer::get_object(const util::uuid& pId)
{
	object_data* data = mObject_manager.get_object_data(pId);
	if (!data)
		return{};
	return{ *this, *data };
}

std::size_t layer::get_object_count() const noexcept
{
	return mObject_manager.get_object_count();
}

component* layer::add_component(const game_object& pObj, const component_type& pType)
{
	const factory& f = get_engine().get_factory();
	component* c = f.create_component(pType, mComponent_manager);
	if (!c)
	{
		log::error() << "Could not create component with id " << pType << log::endm;
		return nullptr;
	}
	c->set_object(pObj);
	mObject_manager.register_component(c);
	return c;
}

component* layer::get_component(const component_type& pType, const util::uuid& pId)
{
	return mComponent_manager.get_component(pType, pId);
}

void layer::remove_component(int pType, const util::uuid& pId)
{
	component* comp = get_component(pType, pId);
	if (comp)
		comp->destroy();
}

context& layer::get_context() const noexcept
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

void layer::clear()
{
	mTime_scale = 1;
	mName.clear();
	mSystems.clear();
	mObject_manager.clear();
	mComponent_manager.clear();
}

engine& layer::get_engine() const noexcept
{
	return mContext.get_engine();
}

} // namespace wge::core
