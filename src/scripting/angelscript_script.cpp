#include <wge/scripting/angelscript_script.hpp>

// Angelscript
#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scripthandle/scripthandle.h>
#include <scriptstdstring/scriptstdstring.h>
#include <scriptarray/scriptarray.h>

namespace wge::scripting
{

script_engine::script_engine()
{
	mEngine = asCreateScriptEngine();
	RegisterStdString(mEngine);
	RegisterScriptArray(mEngine, true);
}

} // namespace wge::scripting
