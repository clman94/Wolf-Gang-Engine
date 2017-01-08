#include <engine/resource.hpp>
#include <engine/utility.hpp>

#include <iostream>

using namespace engine;


resource::resource()
{
	mIs_loaded = false;
}

bool resource::is_loaded()
{
	return mIs_loaded;
}

void resource::set_loaded(bool pIs_loaded)
{
	mIs_loaded = pIs_loaded;
}

void resource_manager::add_resource(resource_type pType, const std::string& pName, std::shared_ptr<resource> pResource)
{
	mResources[pType][pName] = pResource;
}

bool engine::resource_manager::has_resource(resource_type pType, const std::string & pName)
{
	auto find_type = mResources.find(pType);
	if (find_type == mResources.end())
		return false;

	auto& catagory = find_type->second;

	auto find_name = catagory.find(pName);
	if (find_name == catagory.end())
		return false;
	return true;
}

std::shared_ptr<resource> resource_manager::get_resource_cast(resource_type pType, const std::string & pName)
{
	auto find_type = mResources.find(pType);
	if (find_type == mResources.end())
		return nullptr;

	auto& catagory = find_type->second;

	auto find_name = catagory.find(pName);
	if (find_name == catagory.end())
		return nullptr;

	find_name->second->load();

	return find_name->second;
}

void resource_manager::unload_all()
{
	for (auto& i : mResources)
		for (auto& j : i.second)
			j.second->unload();
}

void resource_manager::unload_unused()
{
	for (auto& i : mResources)
		for (auto& j : i.second)
			if (j.second.use_count() <= 1)
				j.second->unload();
}

void resource_manager::ensure_load()
{
	for (auto& i : mResources)
		for (auto& j : i.second)
			if (j.second.use_count() > 1)
				j.second->load();
}
