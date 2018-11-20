#pragma once

#include <string>

#define WGE_SYSTEM(name__, id__) \
	public: \
	static constexpr int SYSTEM_ID = id__; \
	static constexpr const char* SYSTEM_NAME = name__; \
	virtual int get_system_id() const override { return id__; } \
	virtual std::string get_system_name() const override { return name__; }

namespace wge::core
{

class system
{
public:
	virtual ~system() {}
	virtual int get_system_id() const = 0;
	virtual std::string get_system_name() const = 0;
};

} // namespace wge::core
