
#include <rpg/script_system.hpp>

#include <angelscript/add_on/scriptstdstring/scriptstdstring.h>
#include <angelscript/add_on/scriptmath/scriptmath.h>
#include <angelscript/add_on/scriptdictionary/scriptdictionary.h>

#include <engine/parsers.hpp>

#include "../tinyxml2/xmlshortcuts.hpp"

#include <functional>
#include <algorithm>

using namespace rpg;
using namespace AS;

// #########
// angelscript
// #########

void script_system::message_callback(const asSMessageInfo * msg)
{
	log_entry_type type = log_entry_type::error;
	if (msg->type == asEMsgType::asMSGTYPE_INFORMATION)
		type = log_entry_type::info;
	else if (msg->type == asEMsgType::asMSGTYPE_WARNING)
		type = log_entry_type::warning;

	log_print(msg->section, msg->row, msg->col, type, msg->message);
}

std::string
script_context::get_metadata_type(const std::string & pMetadata)
{
	for (auto i = pMetadata.begin(); i != pMetadata.end(); i++)
	{
		if (!parsers::is_letter(*i))
			return std::string(pMetadata.begin(), i);
	}
	return pMetadata;
}

void script_system::script_abort()
{
	auto c_ctx = mCtxmgr.GetCurrentContext();
	assert(c_ctx != nullptr);
	c_ctx->Abort();
}

void script_system::script_create_thread(AS::asIScriptFunction * func, AS::CScriptDictionary * arg)
{
	if (func == 0)
	{
		util::error("Invalid function");
		return;
	}

	// Create a new context for the co-routine
	asIScriptContext *ctx = mCtxmgr.AddContext(mEngine, func);

	// Pass the argument to the context
	ctx->SetArgObject(0, arg);
}

void script_system::script_create_thread_noargs(AS::asIScriptFunction * func)
{
	if (func == 0)
	{
		util::error("Invalid function");
		return;
	}

	// Create a new context for the co-routine
	mCtxmgr.AddContext(mEngine, func);
}

void script_system::script_make_shared(AS::CScriptHandle pHandle, const std::string& pName)
{
	mShared_handles[pName] = pHandle;
}

AS::CScriptHandle script_system::script_get_shared(const std::string& pName)
{
	return mShared_handles[pName];
}

void script_system::load_script_interface()
{
	add_function("int rand()", asFUNCTION(std::rand));
	add_function("void _timer_start(float)", asMETHOD(engine::timer, start), &mTimer);
	add_function("bool _timer_reached()", asMETHOD(engine::timer, is_reached), &mTimer);

	add_function("void create_thread(coroutine @+)", asMETHOD(script_system, script_create_thread_noargs), this);
	add_function("void create_thread(coroutine @+, dictionary @+)", asMETHOD(script_system, script_create_thread), this);

	add_function("void dprint(const string &in)", asMETHOD(script_system, script_debug_print), this);
	add_function("void eprint(const string &in)", asMETHOD(script_system, script_error_print), this);
	add_function("void abort()", asMETHOD(script_system, script_abort), this);

	add_function("void make_shared(ref@, const string&in)", asMETHOD(script_system, script_make_shared), this);
	add_function("ref@ get_shared(const string&in)", asMETHOD(script_system, script_get_shared), this);
}


void script_system::log_print(const std::string& pFile, int pLine, int pCol, log_entry_type pType, const std::string& pMessage)
{
	if (!mLog_file.is_open()) // Open the log file when needed
		mLog_file.open("./data/log.txt");

	std::string type;
	switch (pType)
	{
	case log_entry_type::error:   type = "ERROR";   break;
	case log_entry_type::info:    type = "INFO";    break;
	case log_entry_type::warning: type = "WARNING"; break;
	case log_entry_type::debug:   type = "DEBUG";   break;
	}

	std::string message = pFile;
	message += " ( ";
	message += std::to_string(pLine);
	message += ", ";
	message += std::to_string(pCol);
	message += " ) : ";
	message += type;
	message += " : ";
	message += pMessage;
	message += "\n";

	if (mLog_file.is_open())
		mLog_file << message; // Save to file

	std::cout << message; // Print to console
}

void
script_system::script_debug_print(std::string &pMessage)
{
	if (!is_executing())
	{
		log_print("Unknown", 0, 0, log_entry_type::debug, pMessage);
		return;
	}

	assert(mCtxmgr.GetCurrentContext() != nullptr);
	assert(mCtxmgr.GetCurrentContext()->GetFunction() != nullptr);

	std::string name = mCtxmgr.GetCurrentContext()->GetFunction()->GetName();
	log_print(name, get_current_line(), 0, log_entry_type::debug, pMessage);
}

void script_system::script_error_print(std::string & pMessage)
{
	if (!is_executing())
	{
		log_print("Unknown", 0, 0, log_entry_type::error, pMessage);
		return;
	}

	assert(mCtxmgr.GetCurrentContext() != nullptr);
	assert(mCtxmgr.GetCurrentContext()->GetFunction() != nullptr);

	std::string name = mCtxmgr.GetCurrentContext()->GetFunction()->GetName();
	log_print(name, get_current_line(), 0, log_entry_type::error, pMessage);
}

void
script_system::register_vector_type()
{
	mEngine->RegisterObjectType("vec", sizeof(engine::fvector), asOBJ_VALUE | asGetTypeTraits<engine::fvector>());

	// Constructors and deconstructors
	mEngine->RegisterObjectBehaviour("vec", asBEHAVE_CONSTRUCT, "void f()"
		, asFUNCTION(script_default_constructor<engine::fvector>)
		, asCALL_CDECL_OBJLAST);
	mEngine->RegisterObjectBehaviour("vec", asBEHAVE_CONSTRUCT, "void f(float, float)"
		, asFUNCTIONPR(script_constructor<engine::fvector>, (float, float, void*), void)
		, asCALL_CDECL_OBJLAST);
	mEngine->RegisterObjectBehaviour("vec", asBEHAVE_CONSTRUCT, "void f(const vec&in)"
		, asFUNCTIONPR(script_constructor<engine::fvector>, (const engine::fvector&, void*), void)
		, asCALL_CDECL_OBJLAST);
	mEngine->RegisterObjectBehaviour("vec", asBEHAVE_DESTRUCT, "void f()"
		, asFUNCTION(script_default_deconstructor<engine::fvector>), asCALL_CDECL_OBJLAST);

	// Assignments
	mEngine->RegisterObjectMethod("vec", "vec& opAssign(const vec &in)"
		, asMETHODPR(engine::fvector, operator=, (const engine::fvector&), engine::fvector&)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec& opAddAssign(const vec &in)"
		, asMETHODPR(engine::fvector, operator+=<float>, (const engine::fvector&), engine::fvector&)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec& opSubAssign(const vec &in)"
		, asMETHODPR(engine::fvector, operator-=<float>, (const engine::fvector&), engine::fvector&)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec& opMulAssign(const vec &in)"
		, asMETHODPR(engine::fvector, operator*=<float>, (const engine::fvector&), engine::fvector&)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec& opMulAssign(float)"
		, asMETHODPR(engine::fvector, operator*=, (float), engine::fvector&)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec& opDivAssign(float)"
		, asMETHODPR(engine::fvector, operator/=, (float), engine::fvector&)
		, asCALL_THISCALL);

	// Arithmic
	mEngine->RegisterObjectMethod("vec", "vec opAdd(const vec &in) const"
		, asMETHODPR(engine::fvector, operator+<float>, (const engine::fvector&) const, engine::fvector)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec opSub(const vec &in) const"
		, asMETHODPR(engine::fvector, operator-<float>, (const engine::fvector&) const, engine::fvector)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec opMul(const vec &in) const"
		, asMETHODPR(engine::fvector, operator*<float>, (const engine::fvector&) const, engine::fvector)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec opMul(float) const"
		, asMETHODPR(engine::fvector, operator*<float>, (float) const, engine::fvector)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec opDiv(float) const"
		, asMETHODPR(engine::fvector, operator/<float>, (float) const, engine::fvector)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec opNeg() const"
		, asMETHODPR(engine::fvector, operator-, () const, engine::fvector)
		, asCALL_THISCALL);

	// Distance
	mEngine->RegisterObjectMethod("vec", "float distance() const"
		, asMETHODPR(engine::fvector, distance, () const, float)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "float distance(const vec &in) const"
		, asMETHODPR(engine::fvector, distance, (const engine::fvector&) const, float)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "float manhattan() const"
		, asMETHODPR(engine::fvector, manhattan, () const, float)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "float manhattan(const vec &in) const"
		, asMETHODPR(engine::fvector, manhattan, (const engine::fvector&) const, float)
		, asCALL_THISCALL);

	// Rotate
	mEngine->RegisterObjectMethod("vec", "vec& rotate(float)"
		, asMETHODPR(engine::fvector, rotate, (float), engine::fvector&)
		, asCALL_THISCALL);
	mEngine->RegisterObjectMethod("vec", "vec& rotate(const vec &in, float)"
		, asMETHODPR(engine::fvector, rotate, (const engine::fvector&, float), engine::fvector&)
		, asCALL_THISCALL);

	mEngine->RegisterObjectMethod("vec", "vec& normalize()"
		, asMETHOD(engine::fvector, normalize)
		, asCALL_THISCALL);

	mEngine->RegisterObjectMethod("vec", "vec& floor()"
		, asMETHOD(engine::fvector, floor)
		, asCALL_THISCALL);

	mEngine->RegisterObjectMethod("vec", "float angle() const"
		, asMETHOD(engine::fvector, angle)
		, asCALL_THISCALL);

	// Members
	mEngine->RegisterObjectProperty("vec", "float x", asOFFSET(engine::fvector, x));
	mEngine->RegisterObjectProperty("vec", "float y", asOFFSET(engine::fvector, y));
}

script_system::script_system()
{
	mEngine = asCreateScriptEngine();

	mEngine->SetEngineProperty(asEP_REQUIRE_ENUM_SCOPE, true);
	//mEngine->SetEngineProperty(asEP_BUILD_WITHOUT_LINE_CUES, true);

	mEngine->SetMessageCallback(asMETHOD(script_system, message_callback), this, asCALL_THISCALL);

	RegisterStdString(mEngine);
	RegisterScriptMath(mEngine);
	RegisterScriptArray(mEngine, true);
	RegisterScriptDictionary(mEngine);
	RegisterScriptHandle(mEngine);
	mCtxmgr.RegisterCoRoutineSupport(mEngine);

	register_vector_type();

	load_script_interface();

	mExecuting = false;
}

script_system::~script_system()
{
	mShared_handles.clear();
	mCtxmgr.AbortAll();
	mCtxmgr.~CContextMgr(); // Destroy context manager before releasing engine
	mEngine->ShutDownAndRelease();
}

void script_system::load_context(script_context & pContext)
{
	mCtxmgr.AbortAll();
	mContext = &pContext;
	start_all_with_tag("start");
}

void
script_system::add_function(const char* pDeclaration, const asSFuncPtr& pPtr, void* pInstance)
{
	int r = mEngine->RegisterGlobalFunction(pDeclaration, pPtr, asCALL_THISCALL_ASGLOBAL, pInstance);
	assert(r >= 0);
}

void
script_system::add_function(const char * pDeclaration, const asSFuncPtr& pPtr)
{
	int r = mEngine->RegisterGlobalFunction(pDeclaration, pPtr, asCALL_CDECL);
	assert(r >= 0);
}

void script_system::about_all()
{
	if (is_executing())
		mCtxmgr.PreAbout();
	else
		mCtxmgr.AbortAll();
}

void script_system::start_all_with_tag(const std::string & pTag)
{
	size_t func_count = mContext->mScene_module->GetFunctionCount();
	for (size_t i = 0; i < func_count; i++)
	{
		auto func = mContext->mScene_module->GetFunctionByIndex(i);
		std::string metadata = parsers::remove_trailing_whitespace(mContext->mBuilder.GetMetadataStringForFunc(func));
		if (metadata == pTag)
		{
			mCtxmgr.AddContext(mEngine, func);
		}
	}
}

int
script_system::tick()
{
	mExecuting = true;
	int r = mCtxmgr.ExecuteScripts();
	mExecuting = false;
	return r;
}

int script_system::get_current_line()
{
	if (mCtxmgr.GetCurrentContext())
	{
		return mCtxmgr.GetCurrentContext()->GetLineNumber();
	}
	return 0;
}

AS::asIScriptEngine& rpg::script_system::get_engine()
{
	assert(mEngine != nullptr);
	return *mEngine;
}

bool script_system::is_executing()
{
	return mExecuting;
}

