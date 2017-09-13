#include <engine/resource.hpp>
#include <engine/utility.hpp>

#include <iostream>

using namespace engine;


resource::resource()
{
	mIs_loaded = false;
	mPack = nullptr;
}

resource::~resource()
{

}

bool resource::is_loaded()
{
	return mIs_loaded;
}

void resource::set_resource_pack(pack_stream_factory * pPack)
{
	mPack = pPack;
}

void resource::set_loaded(bool pIs_loaded)
{
	mIs_loaded = pIs_loaded;
}

resource_manager::resource_manager()
{
	mPack = nullptr;
}

void resource_manager::add_resource(resource_type pType, const std::string& pName, std::shared_ptr<resource> pResource)
{
	mResources[pType][pName] = pResource;
	pResource->set_resource_pack(mPack);
}

bool resource_manager::has_resource(resource_type pType, const std::string & pName)
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

std::shared_ptr<resource> resource_manager::get_resource_precast(resource_type pType, const std::string & pName)
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

void resource_manager::set_resource_pack(pack_stream_factory * pPack)
{
	mPack = pPack;
	for (auto i : mResources)
	{
		for (auto j : i.second)
		{
			j.second->set_resource_pack(pPack);
		}
	}
}

std::string resource_manager::get_resource_log() const
{
	std::string val;

	for (auto& i : mResources)
	{
		for (auto& j : i.second)
		{
			val += (j.second->is_loaded() ? "(loaded)   " : "(unloaded) ") + j.first + "\n";
		}
	}

	return val;
}

void resource_manager::add_directory(std::shared_ptr<resource_directory> pDirectory)
{
	mResource_directories.push_back(pDirectory);
}

bool resource_manager::reload_directories()
{
	mResources.clear();
	for (auto i : mResource_directories)
		if (mPack) {
			if (!i->load_pack(*this, *mPack))
				return false;
		}
		else
		{
			if (!i->load(*this))
				return false;
		}
	return true;
}

void resource_manager::clear_directories()
{
	mResources.clear();
	mResource_directories.clear();
}

void resource_manager::ensure_load()
{
	for (auto& i : mResources)
		for (auto& j : i.second)
			if (j.second.use_count() > 1)
				j.second->load();
}
