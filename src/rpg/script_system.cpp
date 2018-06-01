
#include <engine/logger.hpp>
#include <engine/utility.hpp>
#include <engine/filesystem.hpp>

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
	if (engine::fs::exists(util::safe_string(pMsg->section)))
		msg = logger::print(pMsg->section, pMsg->row, pMsg->col, type, pMsg->message);
	else
		msg = logger::print(type, pMsg->message);
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
		logger::print(*this, logger::level::debug, pMessage);
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
		logger::print(*this, logger::level::error, pMessage);
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

	add_method<engine::fvector, engine::fvector&, const engine::fvector&>("vec", operator_method::assign,                &engine::fvector::operator=);
	add_method<engine::fvector, engine::fvector&, const engine::fvector&>("vec", operator_method::additive_assign,       &engine::fvector::operator+=);
	add_method<engine::fvector, engine::fvector&, const engine::fvector&>("vec", operator_method::subtractive_assign,    &engine::fvector::operator-=);
	add_method<engine::fvector, engine::fvector&, const engine::fvector&>("vec", operator_method::multiplicative_assign, &engine::fvector::operator*=);
	add_method<engine::fvector, engine::fvector&, float>                 ("vec", operator_method::multiplicative_assign, &engine::fvector::operator*=);
	add_method<engine::fvector, engine::fvector&, float>                 ("vec", operator_method::dividing_assign,       &engine::fvector::operator/=);
	add_method<engine::fvector, engine::fvector&, const engine::fvector&>("vec", operator_method::dividing_assign,       &engine::fvector::operator/=);

	add_method<engine::fvector, engine::fvector, const engine::fvector&>("vec", operator_method::add,      &engine::fvector::operator+);
	add_method<engine::fvector, engine::fvector, const engine::fvector&>("vec", operator_method::subtract, &engine::fvector::operator-);
	add_method<engine::fvector, engine::fvector, const engine::fvector&>("vec", operator_method::multiply, &engine::fvector::operator*);
	add_method<engine::fvector, engine::fvector, float>                 ("vec", operator_method::multiply, &engine::fvector::operator*);
	add_method<engine::fvector, engine::fvector, float>                 ("vec", operator_method::divide,   &engine::fvector::operator/);
	add_method<engine::fvector, engine::fvector, const engine::fvector&>("vec", operator_method::divide,   &engine::fvector::operator/);
	add_method<engine::fvector, engine::fvector>                        ("vec", operator_method::negative, &engine::fvector::operator-);
	add_method<engine::fvector>                                         ("vec", operator_method::equals,   &engine::fvector::operator==<float>);

	add_method<engine::fvector, float>                        ("vec", "distance" , &engine::fvector::distance);
	add_method<engine::fvector, float, const engine::fvector&>("vec", "distance" , &engine::fvector::distance);
	add_method<engine::fvector, float>                        ("vec", "manhattan", &engine::fvector::manhattan);
	add_method<engine::fvector, float, const engine::fvector&>("vec", "manhattan", &engine::fvector::manhattan);

	add_method<engine::fvector, engine::fvector&, float> ("vec", "rotate", &engine::fvector::rotate);
	add_method<engine::fvector, engine::fvector&, const engine::fvector&, float>("vec", "rotate", &engine::fvector::rotate);
	add_method("vec", "normalize", &engine::fvector::normalize);
	add_method("vec", "floor",     &engine::fvector::floor);
	add_method("vec", "dot", &engine::fvector::dot);
	add_method<engine::fvector, float>                        ("vec", "angle", &engine::fvector::angle);
	add_method<engine::fvector, float, const engine::fvector&>("vec", "angle", &engine::fvector::angle);

	// Members
	add_member("vec", "x", &engine::fvector::x);
	add_member("vec", "y", &engine::fvector::y);
}

void script_system::register_timer_type()
{
	begin_namespace("util");
	add_object<engine::timer>("timer");
	add_method<engine::timer, void, float>("timer", "start", &engine::timer::start);
	add_method("timer", "is_reached", &engine::timer::is_reached);
	end_namespace();
}

void script_system::register_rect_type()
{
	add_object<engine::frect>("rect", true);
	add_constructor<engine::frect, float, float, float, float>("rect");
	add_constructor<engine::frect, const engine::fvector&, const engine::fvector&>("rect");
	add_constructor<engine::frect, const engine::frect&>("rect");

	add_method("rect", "get_offset", &engine::frect::get_offset);
	add_method("rect", "set_offset", &engine::frect::set_offset);
	add_method("rect", "get_size", &engine::frect::get_size);
	add_method("rect", "set_size", &engine::frect::set_size);

	add_method("rect", "get_center", &engine::frect::get_center);
	add_method("rect", "get_corner", &engine::frect::get_corner);

	add_method("rect", operator_method::assign, &engine::frect::operator=);
	add_method("rect", operator_method::multiply, &engine::frect::operator*);

	add_method<engine::frect, bool, const engine::frect&>("rect", "is_intersect", &engine::frect::is_intersect);
	add_method<engine::frect, bool, const engine::fvector&>("rect", "is_intersect", &engine::frect::is_intersect);

	// Members
	add_member("rect", "x", &engine::frect::x);
	add_member("rect", "y", &engine::frect::y);
	add_member("rect", "w", &engine::frect::w);
	add_member("rect", "h", &engine::frect::h);
}

void script_system::register_color_type()
{
	add_object<engine::color>("color", true);
	add_constructor<engine::color, float, float, float>("color");
	add_constructor<engine::color, float, float, float, float>("color");
	add_constructor<engine::color, const engine::color&>("color");

	add_method<engine::color, engine::color&, const engine::color&>("color", operator_method::assign,                &engine::color::operator=);
	add_method<engine::color, engine::color&, const engine::color&>("color", operator_method::additive_assign,       &engine::color::operator+=);
	add_method<engine::color, engine::color&, const engine::color&>("color", operator_method::subtractive_assign,    &engine::color::operator-=);
	add_method<engine::color, engine::color&, const engine::color&>("color", operator_method::multiplicative_assign, &engine::color::operator*=);
	add_method<engine::color, engine::color&, const engine::color&>("color", operator_method::dividing_assign,       &engine::color::operator/=);

	add_method<engine::color, engine::color, const engine::color&>("color", operator_method::add,      &engine::color::operator+);
	add_method<engine::color, engine::color, const engine::color&>("color", operator_method::subtract, &engine::color::operator-);
	add_method<engine::color, engine::color, const engine::color&>("color", operator_method::multiply, &engine::color::operator*);
	add_method<engine::color, engine::color, const engine::color&>("color", operator_method::divide,   &engine::color::operator/);

	// Members
	add_member("color", "r", &engine::color::r);
	add_member("color", "g", &engine::color::g);
	add_member("color", "b", &engine::color::b);
	add_member("color", "a", &engine::color::a);
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
	register_rect_type();
	register_color_type();

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

void script_system::add_enum(const std::string & pName)
{
	mEngine->RegisterEnum(pName.c_str());
}

void script_system::add_enum_value(const std::string & pName, const std::string & pValue_name, int pValue)
{
	mEngine->RegisterEnumValue(pName.c_str(), pValue_name.c_str(), pValue);
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

size_t script_system::get_current_thread()
{
	for (std::size_t i = 0; i < mThread_contexts.size(); i++)
		if (mThread_contexts[i] == mCurrect_thread_context)
			return i;
	return 0;
}

void script_system::begin_namespace(const std::string & pName)
{
	mEngine->SetDefaultNamespace(pName.c_str());
}

void script_system::end_namespace()
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

std::vector<script_system::stack_level_info> script_system::get_stack_info(size_t pThread) const
{
	if (mThread_contexts.empty())
		return{};
	std::vector<stack_level_info> info_list;
	AS::asIScriptContext* ctx = mThread_contexts[pThread]->context;
	info_list.resize(ctx->GetCallstackSize());
	for (size_t i = 0; i < ctx->GetCallstackSize(); i++)
	{
		info_list[i].func = ctx->GetFunction(i);
		info_list[i].line = ctx->GetLineNumber(i, &info_list[i].column, &info_list[i].section);
	}
	return info_list;
}

std::vector<script_system::var_info> script_system::get_var_info(size_t pThread, size_t pStack) const
{
	std::vector<var_info> info_list;
	AS::asIScriptContext* ctx = mThread_contexts[pThread]->context;
	if (ctx->GetThisTypeId(pStack))
	{
		info_list.reserve(ctx->GetVarCount(pStack) + 1);
		var_info this_var;
		this_var.name = "this";
		this_var.pointer = ctx->GetThisPointer();
		this_var.type_id = ctx->GetThisTypeId(pStack);
		this_var.type_decl = mEngine->GetTypeDeclaration(this_var.type_id, true);
		info_list.push_back(this_var);
	}
	else
		info_list.reserve(ctx->GetVarCount(pStack));
	for (size_t i = 0; i < ctx->GetVarCount(pStack); i++)
	{
		var_info var;
		var.name = ctx->GetVarName(i, pStack);
		var.pointer = ctx->GetAddressOfVar(i, pStack);
		var.type_id = ctx->GetVarTypeId(i, pStack);
		var.type_decl = mEngine->GetTypeDeclaration(var.type_id, true);
		info_list.push_back(var);
	}
	return info_list;
}

size_t script_system::get_thread_count() const
{
	return mThread_contexts.size();
}

AS::asIScriptFunction* script_system::get_thread_function(size_t pThread) const
{
	return mThread_contexts[pThread]->context->GetFunction();
}

AS::asITypeInfo* script_system::get_type_info_from_decl(const char * pDecl) const
{
	return mEngine->GetTypeInfoByDecl(pDecl);
}

void script_system::throw_exception(const std::string & pMessage)
{
	mCurrect_thread_context->context->SetException(pMessage.c_str());
}

logger::message logger::print(script_system & pScript_system, level pType, const std::string & pMessage)
{
	logger::message msg(pType);
	msg.row = pScript_system.get_current_line();
	msg.file = pScript_system.get_current_file();
	msg.is_file = true;
	msg.msg = pMessage;

	auto message_data = std::make_shared<script_message_data>();
	message_data->stack_info = pScript_system.get_stack_info(pScript_system.get_current_thread());
	msg.ext = message_data;

	return print(msg);
}
