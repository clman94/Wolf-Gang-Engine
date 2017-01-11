#ifndef ENGINE_RESOURCE_HPP
#define ENGINE_RESOURCE_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>

namespace engine
{

class resource_manager;

class resource
{
public:
	resource();
	~resource() {};
	virtual bool load() = 0;
	virtual bool unload() = 0;
	bool is_loaded();

protected:
	void set_loaded(bool pIs_loaded);

private:
	bool mIs_loaded;
};

class resource_directory
{
public:
	virtual bool load(resource_manager& pResource_manager) = 0;
};

template<typename T1, typename T2>
std::shared_ptr<T1> cast_resource(std::shared_ptr<T2> pSrc)
{
	return std::dynamic_pointer_cast<T1>(pSrc);
}

enum class resource_type
{
	texture,
	sound,
	font,
};

class resource_manager
{
public:
	void add_resource(resource_type pType, const std::string& pName, std::shared_ptr<resource> pResource);
	bool has_resource(resource_type pType, const std::string& pName);

	template<typename T = resource>
	std::shared_ptr<T> get_resource(resource_type pType, const std::string& pName)
	{
		auto res = get_resource_precast(pType, pName);
		return cast_resource<T>(res);
	}

	void add_directory(std::shared_ptr<resource_directory> pDirectory);
	void reload_directories();

	void ensure_load();
	void unload_all();
	void unload_unused();

private:
	std::shared_ptr<resource> get_resource_precast(resource_type pType, const std::string& pName);

	typedef std::map<std::string, std::shared_ptr<resource>> resources_t;
	std::map<resource_type, resources_t> mResources;

	std::vector<std::shared_ptr<resource_directory>> mResource_directories;
};

}
#endif // !ENGINE_RESOURCE_HPP
