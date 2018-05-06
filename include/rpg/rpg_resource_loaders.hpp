#ifndef RPG_RESOURCE_DIRECTORIES_HPP
#define RPG_RESOURCE_DIRECTORIES_HPP

#include <engine/resource.hpp>

namespace rpg
{

class texture_loader :
	public engine::resource_loader
{
public:
	virtual bool load(engine::resource_manager& pResource_manager, const engine::fs::path& mData_filepath) override;
	virtual bool load_pack(engine::resource_manager& pResource_manager, engine::resource_pack& pPack) override;
};

class font_loader :
	public engine::resource_loader
{
public:
	virtual bool load(engine::resource_manager& pResource_manager, const engine::fs::path& mData_filepath) override;
	virtual bool load_pack(engine::resource_manager& pResource_manager, engine::resource_pack& pPack) override;
};

class audio_loader :
	public engine::resource_loader
{
public:
	virtual bool load(engine::resource_manager& pResource_manager, const engine::fs::path& mData_filepath) override;
	virtual bool load_pack(engine::resource_manager& pResource_manager, engine::resource_pack& pPack) override;
};

class script_loader :
	public engine::resource_loader
{
public:
	virtual bool load(engine::resource_manager& pResource_manager, const engine::fs::path& mData_filepath) override;
	virtual bool load_pack(engine::resource_manager& pResource_manager, engine::resource_pack& pPack) override;
};

}

#endif // !RPG_MANAGERS_HPP
