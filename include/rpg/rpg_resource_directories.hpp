#ifndef RPG_RESOURCE_DIRECTORIES_HPP
#define RPG_RESOURCE_DIRECTORIES_HPP

#include <engine/resource.hpp>

namespace rpg
{

class texture_directory :
	public engine::resource_directory
{
public:
	virtual bool load(engine::resource_manager& pResource_manager, const std::string& mData_filepath);
	virtual bool load_pack(engine::resource_manager& pResource_manager, engine::pack_stream_factory& pPack);
};

class font_directory :
	public engine::resource_directory
{
public:
	virtual bool load(engine::resource_manager& pResource_manager, const std::string& mData_filepath);
	virtual bool load_pack(engine::resource_manager& pResource_manager, engine::pack_stream_factory& pPack);
};

class audio_directory :
	public engine::resource_directory
{
public:
	virtual bool load(engine::resource_manager& pResource_manager, const std::string& mData_filepath);
	virtual bool load_pack(engine::resource_manager& pResource_manager, engine::pack_stream_factory& pPack);
};

}

#endif // !RPG_MANAGERS_HPP
