#ifndef RPG_RESOURCE_DIRECTORIES_HPP
#define RPG_RESOURCE_DIRECTORIES_HPP

#include <engine/resource.hpp>

namespace rpg
{

class texture_directory :
	public engine::resource_directory
{
public:
	texture_directory();

	virtual bool load(engine::resource_manager& pResource_manager);
	virtual bool load_pack(engine::resource_manager& pResource_manager, engine::pack_stream_factory& pPack);

	void set_path(const std::string& pPath);
private:
	std::string mPath;
};

class font_directory :
	public engine::resource_directory
{
public:
	font_directory();

	virtual bool load(engine::resource_manager& pResource_manager);
	virtual bool load_pack(engine::resource_manager& pResource_manager, engine::pack_stream_factory& pPack);

	void set_path(const std::string& pPath);
private:
	std::string mPath;
};

class audio_directory :
	public engine::resource_directory
{
public:
	audio_directory();

	virtual bool load(engine::resource_manager& pResource_manager);
	virtual bool load_pack(engine::resource_manager& pResource_manager, engine::pack_stream_factory& pPack);

	void set_path(const std::string& pPath);
private:
	std::string mPath;
};

}

#endif // !RPG_MANAGERS_HPP
