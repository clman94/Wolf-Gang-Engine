#include <wge/core/asset.hpp>
#include <wge/logging/log.hpp>

namespace wge::core
{

asset::asset(const asset_config::ptr& pConfig) :
	mConfig(pConfig)
{
	WGE_ASSERT(pConfig != nullptr);
}

const filesystem::path& asset::get_path() const noexcept
{
	return mPath;
}

void asset::set_path(const filesystem::path& pPath)
{
	mPath = pPath;
}

const util::uuid& asset::get_id() const
{
	return mConfig->get_id();
}

const std::string& asset::get_type() const
{
	return mConfig->get_type();
}

void asset::save() const
{
	on_before_save_config();
	get_config()->save();
}

asset_config::ptr asset::get_config() const noexcept
{
	return mConfig;
}

} // namespace wge::core
