#ifndef RPG_RESOURCE_DIRECTORIES_HPP
#define RPG_RESOURCE_DIRECTORIES_HPP

#include <engine/resource.hpp>

namespace rpg
{

class texture_loader :
	public engine::resource_loader
{
public:
	virtual bool load(engine::resource_manager& pResource_manager, const std::string& mData_filepath);
	virtual bool load_pack(engine::resource_manager& pResource_manager, engine::resource_pack& pPack);
};

class font_loader :
	public engine::resource_loader
{
public:
	virtual bool load(engine::resource_manager& pResource_manager, const std::string& mData_filepath);
	virtual bool load_pack(engine::resource_manager& pResource_manager, engine::resource_pack& pPack);
};

class audio_loader :
	public engine::resource_loader
{
public:
	virtual bool load(engine::resource_manager& pResource_manager, const std::string& mData_filepath);
	virtual bool load_pack(engine::resource_manager& pResource_manager, engine::resource_pack& pPack);
};

}

#endif // !RPG_MANAGERS_HPP
