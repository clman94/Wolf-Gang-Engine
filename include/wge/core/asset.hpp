#pragma once

#include <wge/core/asset_config.hpp>
#include <wge/filesystem/path.hpp>

#include <memory>

namespace wge::core
{

// This is an overridable class for assets.
// Technically, an asset object is meant
// to translate data from the asset configuration.
class asset
{
public:
	template<typename T = asset>
	using tptr = std::shared_ptr<T>;
	using ptr = std::shared_ptr<asset>;
	using wptr = std::weak_ptr<asset>;

	asset(const asset_config::ptr& pConfig);
	virtual ~asset() {}

	// This path will be used to locate this asset
	const filesystem::path& get_path() const noexcept;
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

	// Update and save the assets configuration.
	// If you use the save method in the asset_config object
	// directly, the asset will not have a chance to update
	// any changed settings.
	void save() const;

	asset_config::ptr get_config() const noexcept;

protected:
	virtual void on_before_save_config() const {}

private:
	filesystem::path mPath;
	asset_config::ptr mConfig;
};

template <typename Tto, typename Tfrom>
[[nodiscard]] inline asset::tptr<Tto> cast_asset(const asset::tptr<Tfrom>& pFrom) noexcept
{
	return std::dynamic_pointer_cast<Tto>(pFrom);
}

} // namespace wge::core
