
#include <engine/logger.hpp>
#include <engine/utility.hpp>

#include <rpg/script_system.hpp>

#include "../3rdparty/AngelScript/sdk/add_on/scriptstdstring/scriptstdstring.h"
#include "../3rdparty/AngelScript/sdk/add_on/scriptmath/scriptmath.h"

#include <functional>
#include <algorithm>

using namespace rpg;
using namespace AS;

// #########
// script_system
// #########

void script_system::message_callback(const asSMessageInfo * pMsg)
{

	logger::level type = logger::level::error;
	if (pMsg->type == asEMsgType::asMSGTYPE_INFORMATION)
		type = logger::level::info;
	else if (pMsg->type == asEMsgType::asMSGTYPE_WARNING)
		type = logger::level::warning;

	logger::message msg;
	if (util::safe_string(pMsg->section) == "Unknown")
		msg = logger::print(type, pMsg->message);
	else
		msg = logger::print(pMsg->section, pMsg->row, pMsg->col, type, pMsg->message);
}

void script_system::script_abort()
{
	assert(mCurrect_thread_context != nullptr);
	mCurrect_thread_context->context->Abort();
}

void script_system::script_create_thread(AS::asIScriptFunction * func, AS::CScriptDictionary * arg)
{
	if (!func)
	{
		logger::error("Invalid function");
		return;
	}

	// Create a new context for the co-routine
	asIScriptContext *ctx = create_thread(func)->context;

	// Pass the argument to the context
	ctx->SetArgObject(0, arg);
}

void script_system::script_create_thread_noargs(AS::asIScriptFunction * func)
{
	if (func == 0)
	{
		logger::error("Invalid function");
		return;
	}

	create_thread(func);
}

bool script_system::script_yield()
{
	assert(mCurrect_thread_context != nullptr);
	mCurrect_thread_context->context->Suspend();
	return true;
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
	add_function("rand", &std::rand);

	mEngine->RegisterFuncdef("void coroutine(dictionary@)");
	mEngine->RegisterFuncdef("void coroutine_noargs()");
	add_function("void create_thread(coroutine @+)", asMETHOD(script_system, script_create_thread_noargs), this);
	add_function("void create_thread(coroutine @+, dictionary @+)", asMETHOD(script_system, script_create_thread), this);

	add_function("dprint", &script_system::script_debug_print, this);
	add_function("eprint", &script_system::script_error_print, this);
	add_function("abort", &script_system::script_abort, this);
	add_function("yield", &script_system::script_yield, this);

	add_function("void make_shared(ref@, const string&in)", asMETHOD(script_system, script_make_shared), this);
	add_function("ref@ get_shared(const string&in)", asMETHOD(script_system, script_get_shared), this);
}

void script_system::timeout_callback(AS::asIScriptContext *ctx)
{
	if (mTimeout_timer.is_reached())
	{
		logger::error("Script running too long. (Infinite loop?)");
		ctx->Abort();
		logger::info("In file '" + std::string(ctx->GetFunction()->GetScriptSectionName()) + "' :");
		logger::info("  Script aborted at line " + std::to_string(ctx->GetLineNumber())
			+ " in function '" + std::string(ctx->GetFunction()->GetDeclaration(true, true)) + "'");
	}
}

void script_system::script_debug_print(const std::string &pMessage)
{
	if (!is_executing())
	{
		logger::print("Unknown", 0, logger::level::debug, pMessage);
		return;
	}

	assert(mCurrect_thread_context->context != nullptr);
	assert(mCurrect_thread_context->context->GetFunction() != nullptr);

	logger::print(get_current_file(), get_current_line(), logger::level::debug, pMessage);
}

void script_system::script_error_print(const std::string & pMessage)
{
	if (!is_executing())
	{
		logger::print("Unknown", 0, logger::level::error, pMessage);
		return;
	}

	assert(mCurrect_thread_context->context != nullptr);
	assert(mCurrect_thread_context->context->GetFunction() != nullptr);

	logger::print(get_current_file(), get_current_line(), logger::level::error, pMessage);
}

void script_system::register_vector_type()
{
	add_object<engine::fvector>("vec", true);
	add_constructor<engine::fvector, float, float>("vec");
	add_constructor<engine::fvector, const engine::fvector&>("vec");

	add_method<engine::fvector, engine::fvector&, const engine::fvector&>("vec", operator_method::assign, &engine::fvector::operator=);
	add_method<engine::fvector, engine::fvector&, const engine::fvector&>("vec", operator_method::additive_assign, &engine::fvector::operator+=);
	add_method<engine::fvector, engine::fvector&, const engine::fvector&>("vec", operator_method::subtractive_assign, &engine::fvector::operator-=);
	add_method<engine::fvector, engine::fvector&, const engine::fvector&>("vec", operator_method::multiplicative_assign, &engine::fvector::operator*=);
	add_method<engine::fvector, engine::fvector&, float>                 ("vec", operator_method::multiplicative_assign, &engine::fvector::operator*=);
	add_method<engine::fvector, engine::fvector&, float>                 ("vec", operator_method::dividing_assign, &engine::fvector::operator/=);

	add_method<engine::fvector, engine::fvector, const engine::fvector&>("vec", operator_method::add, &engine::fvector::operator+);
	add_method<engine::fvector, engine::fvector, const engine::fvector&>("vec", operator_method::subtract, &engine::fvector::operator-);
	add_method<engine::fvector, engine::fvector, const engine::fvector&>("vec", operator_method::multiply, &engine::fvector::operator*);
	add_method<engine::fvector, engine::fvector, float>                 ("vec", operator_method::multiply, &engine::fvector::operator*);
	add_method<engine::fvector, engine::fvector, float>                 ("vec", operator_method::divide, &engine::fvector::operator/);
	add_method<engine::fvector, engine::fvector>                        ("vec", operator_method::negative, &engine::fvector::operator-);
	add_method<engine::fvector>                                         ("vec", operator_method::equals, &engine::fvector::operator==<float>);

	add_method<engine::fvector, float>                        ("vec", "distance" , &engine::fvector::distance);
	add_method<engine::fvector, float, const engine::fvector&>("vec", "distance" , &engine::fvector::distance);
	add_method<engine::fvector, float>                        ("vec", "manhattan", &engine::fvector::manhattan);
	add_method<engine::fvector, float, const engine::fvector&>("vec", "manhattan", &engine::fvector::manhattan);

	add_method<engine::fvector, engine::fvector&, float> ("vec", "rotate", &engine::fvector::rotate);
	add_method<engine::fvector, engine::fvector&, const engine::fvector&, float>("vec", "rotate", &engine::fvector::rotate);
	add_method("vec", "normalize", &engine::fvector::normalize);
	add_method("vec", "floor"    , &engine::fvector::floor);
	add_method<engine::fvector, float>                        ("vec", "angle", &engine::fvector::angle);
	add_method<engine::fvector, float, const engine::fvector&>("vec", "angle", &engine::fvector::angle);

	// Members
	add_member("vec", "x", &engine::fvector::x);
	add_member("vec", "y", &engine::fvector::y);
}

void script_system::register_timer_type()
{
	set_namespace("util");
	add_object<engine::timer>("timer");
	add_method<engine::timer, void, float>("timer", "start", &engine::timer::start);
	add_method("timer", "is_reached", &engine::timer::is_reached);
	reset_namespace();
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

	register_vector_type();
	register_timer_type();

	load_script_interface();
}

script_system::~script_system()
{
	mShared_handles.clear();
	abort_all();

	// Just screw it!
	mEngine->ShutDownAndRelease();
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

void script_system::abort_all()
{
	for (auto& i : mThread_contexts)
	{
		i->context->Abort();
		mEngine->ReturnContext(i->context);
		i->context = nullptr;
	}
	mThread_contexts.clear();
	mEngine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE);
}

int script_system::tick()
{
	for (size_t i = 0; i < mThread_contexts.size(); i++)
	{
		mCurrect_thread_context = mThread_contexts[i];
		
		// Timeout feature disabled in release mode to remove overhead
#ifndef LOCKED_RELEASE_MODE

		// 5 second timeout for scripts
		mTimeout_timer.start(5);
		mCurrect_thread_context->context->SetLineCallback(
			AS::asMETHOD(script_system, timeout_callback), this, asCALL_THISCALL);
#endif

		if (mCurrect_thread_context->context->Execute() != AS::asEXECUTION_SUSPENDED)
		{
			if (!mCurrect_thread_context->keep_context)
			{
				mEngine->ReturnContext(mCurrect_thread_context->context);
				mCurrect_thread_context->context = nullptr;
			}
			mThread_contexts.erase(mThread_contexts.begin() + i);
			--i;
		}

		mCurrect_thread_context = nullptr;

		mEngine->GarbageCollect(asGC_ONE_STEP | asGC_DETECT_GARBAGE);
	}

	return mThread_contexts.size();
}

int script_system::get_current_line()
{
	if (mCurrect_thread_context)
	{
		return mCurrect_thread_context->context->GetLineNumber();
	}
	return 0;
}

std::string script_system::get_current_file() const
{
	if (!mCurrect_thread_context)
	{
		logger::error("Could not get script of function");
		return{};
	}

	return mCurrect_thread_context->context->GetFunction()->GetScriptSectionName();
}

void script_system::set_namespace(const std::string & pName)
{
	mEngine->SetDefaultNamespace(pName.c_str());
}

void script_system::reset_namespace()
{
	mEngine->SetDefaultNamespace("");
}

std::shared_ptr<script_system::thread> script_system::create_thread(AS::asIScriptFunction * pFunc
	, bool pKeep_context)
{
	asIScriptContext* context = mEngine->RequestContext();
	if (!context)
		return nullptr;

	if (context->Prepare(pFunc) < 0)
	{
		mEngine->ReturnContext(context);
		return nullptr;
	}

	// Create a new one if necessary
	std::shared_ptr<thread> new_thread(std::make_shared<thread>());
	new_thread->context = context;
	new_thread->keep_context = pKeep_context;
	mThread_contexts.push_back(new_thread);

	return new_thread;
}

bool script_system::is_executing()
{
	return mCurrect_thread_context != nullptr;
}


