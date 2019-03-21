#pragma once

#include <wge/core/behavior.hpp>

#include <angelscript.h>

namespace wge::scripting
{

class script_engine;

class script_behavior_instance :
	public core::behavior_instance
{
public:
	script_behavior_instance(script_engine&);

private:
	asIScriptObject* mObject;
};

class script_behavior :
	public core::behavior
{
public:
	script_behavior(core::asset_config::ptr pConfig, script_engine&) :
		core::behavior(pConfig)
	{

	}
};

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
