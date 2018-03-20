#ifndef ENGINE_RESOURCE_HPP
#define ENGINE_RESOURCE_HPP

#include <string>
#include <memory>
#include <vector>
#include <engine/resource_pack.hpp>

namespace engine
{

class resource_manager;

class resource
{
public:
	resource();
	virtual ~resource() {}
	virtual bool load() = 0;
	virtual bool unload() = 0;
	bool is_loaded();

	void set_name(const std::string& pName);
	const std::string& get_name() const;

	virtual const std::string& get_type() const = 0;

	void set_resource_pack(resource_pack* pPack);

protected:
	bool set_loaded(bool pIs_loaded);
	resource_pack* mPack;
	std::string mName;

private:
	bool mIs_loaded;
};

class resource_loader
{
public:
	virtual bool load(resource_manager& pResource_manager, const std::string& mData_filepath) = 0;
	virtual bool load_pack(resource_manager& pResource_manager, resource_pack& pPack) = 0;
};

template<typename T1, typename T2>
inline std::shared_ptr<T1> cast_resource(std::shared_ptr<T2> pSrc)
{
	return std::dynamic_pointer_cast<T1>(pSrc);
}

class resource_manager
{
public:
	resource_manager();

	void add_resource(std::shared_ptr<resource> pResource);
	bool has_resource(std::shared_ptr<resource> pResource) const;
	bool has_resource(const std::string& pType, const std::string& pName) const;

	template<typename T = resource>
	std::shared_ptr<T> get_resource(const std::string& pType, const std::string& pName)
	{
		auto res = find_resource(pType, pName);
		if (!res)
			return{};
		res->load();
		return cast_resource<T>(res);
	}

	void add_loader(std::shared_ptr<resource_loader> pLoader);
	void remove_loader(std::shared_ptr<resource_loader> pLoader);
	void clear_loaders();

	void ensure_load();
	bool reload_all();
	void unload_all();
	void unload_unused();
	void clear_resources();

	void set_resource_pack(resource_pack* pPack);

	std::string get_resource_log() const;

	void set_data_folder(const std::string& pFilepath);

private:
	std::string mData_filepath;

	resource_pack* mPack;

	std::shared_ptr<resource> find_resource(const std::string& pType, const std::string& pName) const;

	std::vector<std::shared_ptr<resource>> mResources;
	std::vector<std::shared_ptr<resource_loader>> mLoaders;
};

}
#endif // !ENGINE_RESOURCE_HPP
