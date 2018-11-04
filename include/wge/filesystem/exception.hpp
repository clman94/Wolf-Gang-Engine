#pragma once

#include <stdexcept>

namespace wge::filesystem
{

class io_error :
	public std::runtime_error
{
public:
	io_error(const std::string& pWhat) :
		std::runtime_error(pWhat)
	{}
};

} // namespace wge::filesystem
