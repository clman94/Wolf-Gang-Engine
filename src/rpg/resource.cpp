#include <engine/resource.hpp>
#include <engine/utility.hpp>

using namespace engine;

resource::resource()
{
	mIs_loaded = false;
	mPack = nullptr;
}

bool resource::is_loaded()
{
	return mIs_loaded;
}

void resource::set_name(const std::string & pName)
{
	mName = pName;
}

const std::string & resource::get_name() const
{
	return mName;
}

void resource::set_resource_pack(resource_pack * pPack)
{
	mPack = pPack;
}

bool resource::set_loaded(bool pIs_loaded)
{
	mIs_loaded = pIs_loaded;
	return pIs_loaded;
}

resource_manager::resource_manager()
{
	mPack = nullptr;
}

void resource_manager::add_resource(std::shared_ptr<resource> pResource)
{
	mResources.push_back(pResource);
	pResource->set_resource_pack(mPack);
}

bool resource_manager::has_resource(std::shared_ptr<resource> pResource) const
{
	for (auto& i : mResources)
		if (i == pResource)
			return true;
	return false;
}

bool resource_manager::has_resource(const std::string& pType, const std::string & pName) const
{
	return find_resource(pType, pName) != nullptr;
}

std::shared_ptr<resource> resource_manager::find_resource(const std::string& pType, const std::string& pName) const
{
	for (auto& i : mResources)
		if (i->get_type() == pType && i->get_name() == pName)
			return i;
	return {};
}

void resource_manager::unload_all()
{
	for (auto& i : mResources)
		 i->unload();
}

void resource_manager::unload_unused()
{
	for (auto& i : mResources)
		if (i.use_count() <= 1)
			i->unload();
}

void resource_manager::clear_resources()
{
	mResources.clear();
}

void resource_manager::set_resource_pack(resource_pack * pPack)
{
	mPack = pPack;
	for (auto& i : mResources)
		 i->set_resource_pack(pPack);
}

std::string resource_manager::get_resource_log() const
{
	std::string val;
	for (auto& i : mResources)
		 val += (i->is_loaded() ? "(loaded)   [" : "(unloaded) [") + i->get_type() + "] " + i->get_name() + "\n";
	return val;
}

void engine::resource_manager::set_data_folder(const std::string & pFilepath)
{
	mData_filepath = pFilepath;
}

void resource_manager::add_loader(std::shared_ptr<resource_loader> pDirectory)
{
	mLoaders.push_back(pDirectory);
}

void resource_manager::remove_loader(std::shared_ptr<resource_loader> pLoader)
{
	for (size_t i = 0; i < mLoaders.size(); i++)
	{
		if (mLoaders[i] == pLoader)
		{
			mLoaders.erase(mLoaders.begin() + i);
			--i;
		}
	}
}

bool resource_manager::reload_all()
{
	mResources.clear();
	for (auto& i : mLoaders)
	{
		if (mPack)
		{
			if (!i->load_pack(*this, *mPack))
				return false;
		}
		else
		{
			if (!i->load(*this, mData_filepath))
				return false;
		}
	}
	return true;
}

void resource_manager::clear_loaders()
{
	mLoaders.clear();
}

void resource_manager::ensure_load()
{
	for (auto& i : mResources)
		if (i.use_count() > 1)
			i->load();
}
