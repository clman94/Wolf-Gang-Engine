#pragma once

#include <wge/core/asset_config.hpp>
#include <wge/filesystem/path.hpp>

#include <memory>

namespace wge::core
{

// This is an overridable class for custom
// asset data. If you don't wan't to specialize any data,
// you don't have to override it.
class asset
{
public:
	template<typename T = asset>
	using tptr = std::shared_ptr<T>;
	using ptr = std::shared_ptr<asset>;

	asset(asset_config::ptr pConfig);

	// This path will be used to locate this asset
	const filesystem::path& get_path() const;
	void set_path(const filesystem::path& pPath);

	asset_uid get_id() const;

	const std::string& get_type() const;

	// Load this asset's data
	virtual void load() {}

	// Unload any data
	virtual void unload() {}

	// Returns true when this asset is ready to be used.
	// Defaults true if this wasn't overridden.
	virtual bool is_loaded() const
	{
		return true;
	}

	asset_config::ptr get_config() const;

private:
	filesystem::path mPath;
	asset_config::ptr mConfig;
};

template <typename Tto, typename Tfrom>
asset::tptr<Tto> cast_asset(asset::tptr<Tfrom> pFrom)
{
	return std::dynamic_pointer_cast<Tto>(pFrom);
}

}