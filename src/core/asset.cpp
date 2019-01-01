#include <wge/core/asset.hpp>
#include <wge/logging/log.hpp>

namespace wge::core
{

asset::asset(const asset_config::ptr& pConfig) :
	mConfig(pConfig)
{
	WGE_ASSERT(pConfig != nullptr);
}

const filesystem::path& asset::get_path() const
{
	return mPath;
}

void asset::set_path(const filesystem::path& pPath)
{
	mPath = pPath;
}

asset_uid asset::get_id() const
{
	return mConfig->get_id();
}

const std::string& asset::get_type() const
{
	return mConfig->get_type();
}

asset_config::ptr asset::get_config() const
{
	return mConfig;
}

} // namespace wge::core
