/// Considering more crazy resource management but probably not right now...

#include <map>
#include <memory>
#include <cassert>

enum class resource_type
{
	texture,
	sound,
	music,
	font,
	misc,
};

class resource
{
public:
	virtual ~resource() {}
	virtual void load() = 0;
	virtual void unload() = 0;
	virtual resource_type get_type() const = 0;

	bool is_loaded() const
	{
		return mIs_loaded;
	}

protected:
	void set_loaded(bool pIs_loaded)
	{
		mIs_loaded = pIs_loaded;
	}

private:
	bool mIs_loaded;
};
typedef std::shared_ptr<resource> shared_resource;

template<class T>
class resource_reference
{
public:
	resource_reference()
	{
		mPtr = nullptr;
	}
	resource_reference(shared_resource pResource)
	{
		assert(pResource != nullptr);
		T* ptr = dynamic_cast<T*>(pResource.get());
		if (mPtr)
			mRef = pResource;
	}

	bool is_valid()
	{
		return mPtr != nullptr;
	}

	T& get()
	{
		assert(mPtr != nullptr);
		return *mPtr;
	}

	operator T&()
	{
		return get();
	}

	T* operator ->()
	{
		return &get();
	}

private:
	shared_resource mRef;
	T* mPtr;
};

class resource_manager
{
public:
	template<class T>
	resource_reference<T> get(const std::string& pId, resource_type pType)
	{
		auto find = get_item(pId, pType);
		if (!find)
			return{}; // None found
		if (!find->is_loaded())
			find->load(); // Load if needed
		return find;
	}

	template<class T>
	void get(resource_reference<T>& pRef, const std::string& pId)
	{
		pRef = get<T>(pId, pRef->get_type());
	}

	void add(shared_resource pResource, const std::string& pId)
	{
		resource_type type = pResource->get_type();
		auto find = get_item(pId, type);
		if (find)
			return; // Resource already exists
		resources[std::make_tuple(type, pId)] = pResource;
	}

	void unload_unused()
	{
		for (auto& i : resources)
			if (i.second.use_count() == 1 &&
				i.second->is_loaded())
				i.second->unload();
	}

private:

	shared_resource get_item(const std::string& pId, resource_type pType)
	{
		auto find = resources.find(std::make_tuple(pType, pId));
		if (find != resources.end())
			return find->second;
		return{};
	}

	std::map<std::tuple<resource_type, std::string>, shared_resource> resources;
};

