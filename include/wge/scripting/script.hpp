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
};

} // namespace wge::scripting
