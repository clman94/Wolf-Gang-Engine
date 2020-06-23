#pragma once

#include <wge/util/json_helpers.hpp>
#include <wge/core/asset_location.hpp>

namespace wge::core
{

class resource
{
public:
	using uptr = std::unique_ptr<resource>;

	virtual ~resource() {}

	void set_location(const asset_location::ptr& pLocation)
	{
		mLocation = pLocation;
	}

	bool has_location() const noexcept
	{
		return mLocation != nullptr;
	}

	asset_location::ptr get_location() const noexcept
	{
		return mLocation;
	}

	void load(const asset_location::ptr& pLocation)
	{
		set_location(pLocation);
		load();
	}

	virtual void load() {}
	virtual void unload() {}

	virtual bool is_loaded() const
	{
		return true;
	}

	virtual void save() {}

	// Serialize any settings from this resource.
	virtual json serialize_data() const { return{}; }
	virtual void deserialize_data(const json& pJson) {}

private:
	asset_location::ptr mLocation;
};

} // namespace wge::core
