#pragma once

#include <wge/core/asset.hpp>
#include <sol/protected_function.hpp>

namespace wge::scripting
{

class script :
	public core::resource
{
public:
	using handle = core::resource_handle<script>;

	std::string source;
	sol::protected_function function;
	std::vector<std::pair<int, std::string>> function_list;

	bool is_compiled() const noexcept
	{
		return function.valid();
	}

	void parse_function_list();

	virtual void load() override;
	virtual void save() override;

	// Scripts are used frequently as secondary assets, so these are utility functions for creating and loading them.
	// Creates a new script file.
	static handle create_secondary_asset(const core::asset::ptr& pParent, const std::string& pName, const std::string& pDefault_text, const core::asset_id& pCustom_id = {});
	// Loads an existing script file.
	static handle load_secondary_asset(const core::asset::ptr& pParent, const std::string& pName, const core::asset_id& pId);
};

} // namespace wge::scripting
