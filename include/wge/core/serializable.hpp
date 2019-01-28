#pragma once

#include <wge/core/asset.hpp>
#include <wge/core/serialize_type.hpp>
#include <wge/logging/log.hpp>
#include <wge/util/json_helpers.hpp>

namespace wge::core
{

class serialized_asset :
	public asset
{
public:
	using ptr = asset::tptr<serialized_asset>;
	serialized_asset(const asset_config::ptr& pConfig) :
		asset(pConfig)
	{}

	const json& get_data() const
	{
		return get_config()->get_metadata();
	}
};

// Derive from this to create a serializable object.
// It will automatically create a new asset containing the 
// serialized data.
class serializable
{
public:
	virtual ~serializable() {}

	virtual json serialize(serialize_type pType) const = 0;
	virtual void deserialize(const json& pJson) = 0;

	void deserialize_asset(const serialized_asset::ptr& pAsset) noexcept
	{
		mAsset = pAsset;
		deserialize(pAsset->get_config()->get_metadata());
	}

	void set_asset(const serialized_asset::ptr& pAsset) noexcept
	{
		mAsset = pAsset;
	}

	const serialized_asset::ptr& get_asset() const noexcept
	{
		return mAsset;
	}

	bool is_asset() const noexcept
	{
		return mAsset != nullptr;
	}

	virtual const char* get_asset_type() const noexcept = 0;

private:
	serialized_asset::ptr mAsset;
};

} // namespace wge::core
