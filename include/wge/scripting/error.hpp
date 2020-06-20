#pragma once

#include <wge/util/uuid.hpp>

#include <string>

namespace wge::scripting
{

struct error_info
{
	std::string source;
	std::string message;
	int line = 0;

	bool operator==(const error_info& pOther) const noexcept
	{
		return source == pOther.source &&
			message == pOther.message && line == pOther.line;
	}

	bool operator!=(const error_info& pOther) const noexcept
	{
		return !operator==(pOther);
	}
};

struct runtime_error_info : error_info
{
	util::uuid asset_id;
};

} // namespace wge::scripting
