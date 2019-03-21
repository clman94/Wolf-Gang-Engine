#pragma once

#include <angelscript.h>

namespace wge::scripting
{
class script_engine
{
public:
	script_engine();

	void compile_scripts()
	{

	}

	void request_recompile() noexcept
	{
		mNeeds_recompile = true;
	}

private:
	bool mNeeds_recompile{ true };
	asIScriptEngine* mEngine;
};

} // namespace wge::scripting
