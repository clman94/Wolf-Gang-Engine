#pragma once

#include <wge/core/system.hpp>

namespace wge::core
{

class context
{
public:
	core::component_factory& get_component_factory()
	{
		return mFactory;
	}

	template <typename T>
	T* get_system() const
	{
		return dynamic_cast<T*>(get_system(T::SYSTEM_ID));
	}

	system* get_system(int pID) const
	{
		for (auto i : mSystems)
			if (i->get_system_id() == pID)
				return i;
		return nullptr;
	}
	system* get_system(const std::string& pName) const
	{
		for (auto i : mSystems)
			if (i->get_system_name() == pName)
				return i;
		return nullptr;
	}

	void add_system(system* pSystem)
	{
		assert(!get_system(pSystem->get_system_id()));
		mSystems.push_back(pSystem);
	}

private:
	core::component_factory mFactory;
	std::vector<system*> mSystems;
};

}