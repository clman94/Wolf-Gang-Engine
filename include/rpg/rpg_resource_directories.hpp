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

	bool load(engine::resource_manager& pResource_manager);

	void set_path(const std::string& pPath);
private:
	std::string mPath;
};

class soundfx_directory :
	public engine::resource_directory
{
public:
	soundfx_directory();

	bool load(engine::resource_manager& pResource_manager);

	void set_path(const std::string& pPath);
private:
	std::string mPath;
};

}

#endif // !RPG_MANAGERS_HPP
