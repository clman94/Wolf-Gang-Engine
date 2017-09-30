#include <rpg/rpg.hpp>

#include <engine/parsers.hpp>
#include <engine/log.hpp>

#include "../xmlshortcuts.hpp"

#include <algorithm>
#include <fstream>
#include <engine/resource_pack.hpp>

using namespace rpg;

// #########
// script_context
// #########

static int add_section_from_pack(const engine::encoded_path& pPath, engine::pack_stream_factory& pPack,  AS::CScriptBuilder& pBuilder)
{
	auto data = pPack.read_all(pPath);
	if (data.empty())
		return -1;
	return pBuilder.AddSectionFromMemory(pPath.string().c_str(), &data[0], data.size());
}

static int pack_include_callback(const char *include, const char *from, AS::CScriptBuilder *pBuilder, void *pUser)
{
	engine::pack_stream_factory* pack = reinterpret_cast<engine::pack_stream_factory*>(pUser);
	auto path = engine::encoded_path(from).parent() / engine::encoded_path(include);
	return add_section_from_pack(path, *pack, *pBuilder);
}

scene_script_context::scene_script_context() :
	mScene_module(nullptr)
{}

scene_script_context::~scene_script_context()
{
}

void scene_script_context::set_path(const std::string & pFilepath)
{
	mScript_path = pFilepath;
}

bool scene_script_context::load()
{
	assert(!mScript_path.empty());
	return build_script(mScript_path);
}

bool scene_script_context::unload()
{
	clean();
	return true;
}

void scene_script_context::set_script_system(script_system & pScript)
{
	mScript = &pScript;
}

bool scene_script_context::build_script(const std::string & pPath)
{
	logger::info("Compiling script '" + pPath + "'...");

	clean();

	mBuilder.SetIncludeCallback(nullptr, nullptr);

	mBuilder.StartNewModule(&mScript->get_engine(), pPath.c_str());
	mBuilder.AddSectionFromMemory("scene_commands", defs::INTERNAL_SCRIPTS_INCLUDE.c_str());
	mBuilder.AddSectionFromFile(pPath.c_str());
	if (mBuilder.BuildModule())
	{
		logger::error("Failed to load scene script");
		return false;
	}
	mScene_module = mBuilder.GetModule();

	parse_wall_group_functions();

	logger::info("Script compiled");
	return true;
}

bool scene_script_context::build_script(const std::string & pPath, engine::pack_stream_factory & pPack)
{
	logger::info("Compiling script '" + pPath + "'...");

	clean();

	mBuilder.SetIncludeCallback(pack_include_callback, &pPack);

	mBuilder.StartNewModule(&mScript->get_engine(), pPath.c_str());

	if (add_section_from_pack(defs::INTERNAL_SCRIPTS_PATH.string(), pPack, mBuilder) < 0)
		return false;
	bool succ = add_section_from_pack(pPath, pPack, mBuilder) >= 0;

	if (!succ || mBuilder.BuildModule() < 0)
	{
		logger::error("Failed to load scene script");
		return false;
	}
	mScene_module = mBuilder.GetModule();

	parse_wall_group_functions();

	logger::info("Script compiled");
	return false;
}

std::string scene_script_context::get_metadata_type(const std::string & pMetadata)
{
	for (auto i = pMetadata.begin(); i != pMetadata.end(); i++)
	{
		if (!parsers::is_letter(*i))
			return std::string(pMetadata.begin(), i);
	}
	return pMetadata;
}

bool scene_script_context::is_valid() const
{
	return mScene_module.has_value();
}

void scene_script_context::clean()
{
	mTrigger_functions.clear();

	mWall_group_functions.clear();

	if (mScene_module)
	{
		mScene_module->Discard();
		mScene_module = nullptr;
	}
}

void scene_script_context::start_all_with_tag(const std::string & pTag)
{
	auto funcs = get_all_with_tag(pTag);

	logger::info("Calling all functions with tag '" + pTag + "'...");
	logger::sub_routine _srtn;

	for (auto& i : funcs)
	{
		logger::info("Calling '" + std::string(i->get_function()->GetDeclaration())
			+ "' in '" + i->get_function()->GetModuleName() + "'");
		i->call();
	}
}

std::vector<std::shared_ptr<script_function>> scene_script_context::get_all_with_tag(const std::string & pTag)
{
	std::vector<std::shared_ptr<script_function>> ret;
	size_t func_count = mScene_module->GetFunctionCount();
	for (size_t i = 0; i < func_count; i++)
	{
		auto func = mScene_module->GetFunctionByIndex(i);
		std::string metadata = parsers::remove_trailing_whitespace(mBuilder.GetMetadataStringForFunc(func));
		if (metadata == pTag)
		{
			std::shared_ptr<script_function> sfunc(new script_function);
			sfunc->set_function(func);
			sfunc->set_script_system(*mScript);
			ret.push_back(sfunc);
		}
	}
	return ret;
}

void scene_script_context::clean_globals()
{
	mScene_module->ResetGlobalVars();
}

const std::vector<scene_script_context::wall_group_function>& scene_script_context::get_wall_group_functions() const
{
	return mWall_group_functions;
}


void scene_script_context::parse_wall_group_functions()
{
	logger::info("Binding functions to wall groups...");
	size_t func_count = mScene_module->GetFunctionCount();
	for (size_t i = 0; i < func_count; i++)
	{
		auto as_function = mScene_module->GetFunctionByIndex(i);
		const std::string metadata = parsers::remove_trailing_whitespace(mBuilder.GetMetadataStringForFunc(as_function));
		const std::string type = get_metadata_type(metadata);

		if (type == "group")
		{
			if (metadata == type) // There is no specified group name
			{
				logger::warning("Group name is not specified in function '" + std::string(as_function->GetDeclaration()) + "'");
				continue;
			}

			std::shared_ptr<script_function> function(new script_function);
			function->set_script_system(*mScript);
			function->set_function(as_function);

			wall_group_function wgf;
			wgf.function = function;

			mTrigger_functions[as_function->GetDeclaration(true, true)].swap(function);

			const std::string group(metadata.begin() + type.length() + 1, metadata.end());
			wgf.group = group;

			mWall_group_functions.push_back(wgf);
		}
	}

	logger::info(std::to_string(mWall_group_functions.size()) + " function(s) bound");
}

// #########
// game
// #########

game::game()
{
	mExit = false;
	mIs_ready = false;
	load_script_interface();
	mSlot = 0;

	mScene.set_resource_manager(mResource_manager);

	mResource_manager.add_directory(std::make_shared<texture_directory>());
	mResource_manager.add_directory(std::make_shared<font_directory>());
	mResource_manager.add_directory(std::make_shared<audio_directory>());
}

game::~game()
{
	logger::info("Destroying game");
	mScene.clean();
}

engine::fs::path game::get_slot_path(size_t pSlot)
{
	return defs::DEFAULT_SAVES_PATH / ("slot_" + std::to_string(pSlot) + ".xml");
}

void game::save_game()
{
	const std::string path = get_slot_path(mSlot).string();
	logger::info("Saving game...");
	logger::sub_routine _srtn;
	mSave_system.new_save();

	mSave_system.save_flags(mFlags);
	mSave_system.save_scene(mScene);
	mSave_system.save(path);
	logger::info("Game saved to '" + path + "'");
}

void game::open_game()
{
	const std::string path = get_slot_path(mSlot).string();
	if (!mSave_system.open_save(path))
	{
		logger::error("Invalid slot '" + std::to_string(mSlot) + "'");
		return;
	}
	logger::info("Opening game...");
	mFlags.clean();
	mSave_system.load_flags(mFlags);
	if (mScript.is_executing())
	{
		mScene_load_request.request_load(mSave_system.get_scene_path());
		mScene_load_request.set_player_position(mSave_system.get_player_position());
	}
	else
	{
		mScene.load_scene(mSave_system.get_scene_path());
		mScene.get_player().set_position(mSave_system.get_player_position());
	}

	logger::info("Loaded " + std::to_string(mFlags.get_count()) + " flag(s)");
	logger::info("Game opened from '" + path + "'");
}

bool game::is_slot_used(size_t pSlot)
{
	const std::string path = get_slot_path(pSlot).string();
	std::ifstream stream(path.c_str());
	return stream.good();
}

void game::set_slot(size_t pSlot)
{
	mSlot = pSlot;
}

size_t game::get_slot()
{
	return mSlot;
}

void game::abort_game()
{
	mExit = true;
}

void game::script_load_scene(const std::string & pName)
{
	logger::info("Requesting scene load '" + pName + "'");
	mScene_load_request.request_load(pName);
}

void game::script_load_scene_to_door(const std::string & pName, const std::string & pDoor)
{
	logger::info("Requesting scene load '" + pName + "'");
	mScene_load_request.request_load(pName);
	mScene_load_request.set_player_position(pDoor);
}

void game::script_load_scene_to_position(const std::string & pName, engine::fvector pPosition)
{
	logger::info("Requesting scene load '" + pName + "'");
	mScene_load_request.request_load(pName);
	mScene_load_request.set_player_position(pPosition);
}

int game::script_get_int_value(const std::string & pPath) const
{
	auto val = mSave_system.get_int_value(pPath);
	if (!val)
	{
		logger::warning("Value '" + pPath + "' does not exist");
		return 0;
	}
	return *val;
}

float game::script_get_float_value(const std::string & pPath) const
{
	auto val = mSave_system.get_float_value(pPath);
	if (!val)
	{
		logger::warning("Value '" + pPath + "' does not exist");
		return 0;
	}
	return *val;
}

std::string game::script_get_string_value(const std::string & pPath) const
{
	auto val = mSave_system.get_string_value(pPath);
	if (!val)
	{
		logger::warning("Value '" + pPath + "' does not exist");
		return{};
	}
	return *val;
}

bool game::script_set_int_value(const std::string & pPath, int pValue)
{
	return mSave_system.set_value(pPath, pValue);
}

bool game::script_set_float_value(const std::string & pPath, float pValue)
{
	return mSave_system.set_value(pPath, pValue);
}

bool game::script_set_string_value(const std::string & pPath, const std::string & pValue)
{
	return mSave_system.set_value(pPath, pValue);
}

AS_array<std::string>* game::script_get_director_entries(const std::string & pPath)
{
	const auto entries = mSave_system.get_directory_entries(pPath);

	auto& engine = mScript.get_engine();

	AS::asITypeInfo* type = engine.GetTypeInfoByDecl("array<string>");
	AS::CScriptArray* arr = AS::CScriptArray::Create(type, entries.size());
	for (size_t i = 0; i < entries.size(); i++)
	{
		arr->SetValue(i, (void*)&entries[i]);
	}

	return static_cast<AS_array<std::string>*>(arr);
}

bool game::script_remove_value(const std::string & pPath)
{
	return mSave_system.remove_value(pPath);
}

bool game::script_has_value(const std::string & pPath)
{
	return mSave_system.has_value(pPath);
}

float game::script_get_tile_size()
{
	return mScene.get_world_node().get_unit();
}

void
game::load_script_interface()
{
	logger::info("Loading script interface...");
	mScript.add_function("get_delta", &game::get_delta, this);

	mScript.add_function("is_triggered", &engine::controls::is_triggered, &mControls);

	mScript.add_function("save_game", &game::save_game, this);
	mScript.add_function("open_game", &game::open_game, this);
	mScript.add_function("abort_game", &game::abort_game, this);
	mScript.add_function("get_slot", &game::get_slot, this);
	mScript.add_function("set_slot", &game::set_slot, this);
	mScript.add_function("is_slot_used", &game::is_slot_used, this);
	mScript.add_function("load_scene", &game::script_load_scene, this);
	mScript.add_function("load_scene", &game::script_load_scene_to_door, this);
	mScript.add_function("load_scene", &game::script_load_scene_to_position, this);

	mScript.set_namespace("values");
	mScript.add_function("get_int", &game::script_get_int_value, this);
	mScript.add_function("get_float", &game::script_get_float_value, this);
	mScript.add_function("get_string", &game::script_get_string_value, this);
	mScript.add_function("set", &game::script_set_int_value, this);
	mScript.add_function("set", &game::script_set_float_value, this);
	mScript.add_function("set", &game::script_set_string_value, this);
	mScript.add_function("get_entries", &game::script_get_director_entries, this);
	mScript.add_function("remove", &game::script_remove_value, this);
	mScript.add_function("exists", &game::script_has_value, this);
	mScript.reset_namespace();

	mScript.add_function("get_tile_size", &game::script_get_tile_size, this);

	mFlags.load_script_interface(mScript);
	mScene.load_script_interface(mScript);
	logger::info("Script interface loaded");
}

void game::load_icon()
{
	get_renderer()->set_icon((mData_directory / "icon.png").string());
}

void game::load_icon_pack()
{
	auto data = mPack.read_all("icon.png");
	get_renderer()->set_icon(data);
}

#ifndef LOCKED_RELEASE_MODE
void game::load_terminal_interface(engine::terminal_system& pTerminal)
{
	mScene.load_terminal_interface(pTerminal);

	mGroup_flags = std::make_shared<engine::terminal_command_group>();
	mGroup_flags->set_root_command("flags");
	mGroup_flags->add_command("set",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.empty())
		{
			logger::error("Not enough arguments");
			logger::warning("flags set <Flag>");
			return false;
		}
		mFlags.set_flag(pArgs[0]);
		logger::info("Flag '" + pArgs[0].get_raw() + "' has been set");
		return true;
	}, "<Flag> - Create flag");

	mGroup_flags->add_command("unset",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.empty())
		{
			logger::error("Not enough arguments");
			logger::warning("flags unset <Flag>");
			return false;
		}
		mFlags.unset_flag(pArgs[0]);
		logger::info("Flag '" + pArgs[0].get_raw() + "' has been unset");
		return true;
	}, "<Flag> - Remove flag");

	mGroup_flags->add_command("clear",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		mFlags.clean();
		logger::info("All flags cleared");
		return true;
	}, "- Remove all flags");

	mGroup_game = std::make_shared<engine::terminal_command_group>();
	mGroup_game->set_root_command("game");
	mGroup_game->add_command("reset",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		return restart_game();
	}, "- Reset game");

	mGroup_game->add_command("load",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.empty())
		{
			logger::error("Not enough arguments");
			return false;
		}
		if (!engine::fs::exists(pArgs[0].get_raw()))
		{
			logger::error("Path '" + pArgs[0].get_raw() + "' does not exist");
			return false;
		}
		mData_directory = pArgs[0].get_raw();
		return restart_game();
	}, "<directory> - Load a game data folder (Default: data)");

	mGroup_global1 = std::make_shared<engine::terminal_command_group>();
	mGroup_global1->add_command("help",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		std::cout << pTerminal.generate_help();
		return true;
	}, "- Display this help");

	mGroup_global1->add_command("pack",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		std::string destination = "./data.pack";
		bool overwrite = false;
		for (auto& i : pArgs)
			if (i.get_raw() == "-o")
				overwrite = true;
			else
				destination = i.get_raw();

		if (engine::fs::exists(destination) && !overwrite)
		{
			logger::error("Pack file '" + destination + "' already exists. Please specify '-o' option to overwrite.");
			return false;
		}

		logger::info("Packing data folder to '" + destination + "'");
		bool suc = engine::create_resource_pack("data", destination);
		logger::info("Packing completed");
		return suc;
	}, "[Destination] [-o] - Pack data folder to a pack file for releasing your game");

	mGroup_slot = std::make_shared<engine::terminal_command_group>();
	mGroup_slot->set_root_command("slot");
	mGroup_slot->add_command("save",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.empty())
		{
			logger::error("Not enough arguments");
			return false;
		}

		size_t slot = 0;
		try {
			slot = static_cast<size_t>(util::min(util::to_numeral<int>(pArgs[0].get_raw()), 0));
		}
		catch (...)
		{
			logger::error("Failed to parse interval");
			return false;
		}

		set_slot(slot);
		save_game();

		return true;
	}, "<Slot #> - Save game to a slot");

	mGroup_slot->add_command("open",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.empty())
		{
			logger::error("Not enough arguments");
			return false;
		}

		size_t slot = 0;
		try {
			slot = static_cast<size_t>(util::min(util::to_numeral<int>(pArgs[0].get_raw()), 0));
		}
		catch (...)
		{
			logger::error("Failed to parse interval");
			return false;
		}

		set_slot(slot);
		open_game();

		return true;
	}, "<Slot #> - Open game from slot");

	pTerminal.add_group(mGroup_flags);
	pTerminal.add_group(mGroup_game);
	pTerminal.add_group(mGroup_global1);
	pTerminal.add_group(mGroup_slot);
}
scene & game::get_scene()
{
	return mScene;
}
engine::resource_manager & rpg::game::get_resource_manager()
{
	return mResource_manager;
}
const engine::fs::path & rpg::game::get_source_path() const
{
	return mData_directory;
}
#endif

float game::get_delta()
{
	return get_renderer()->get_delta();
}

bool game::load(engine::fs::path pData_dir)
{
	using namespace tinyxml2;
	
	mData_directory = pData_dir;

	game_settings_loader settings;
	if (!engine::fs::is_directory(pData_dir)) // This is a package
	{
		logger::info("Loading settings from pack...");
		if (!mPack.open(pData_dir.string()))
		{
			logger::error("Could not load pack");
			return (mIs_ready = false, false);
		}

		const auto settings_data = mPack.read_all("game.xml");
		
		if (!settings.load_memory(&settings_data[0], settings_data.size()))
			return (mIs_ready = false, false);

		mResource_manager.set_resource_pack(&mPack);
		mScene.set_resource_pack(&mPack);

		load_icon_pack();
	}
	else // Data folder
	{
		logger::info("Loading settings from data folder...");
		std::string settings_path = (pData_dir / "game.xml").string();
		if (!settings.load(settings_path, pData_dir.string() + "/"))
			return (mIs_ready = false, false);

		mResource_manager.set_data_folder(pData_dir.string());
		mResource_manager.set_resource_pack(nullptr);
		mScene.set_resource_pack(nullptr);

		load_icon();
	}

	logger::info("Settings loaded");

	get_renderer()->set_target_size(settings.get_screen_size());

	mControls = settings.get_key_bindings();

	logger::info("Loading Resources...");

	if (!mResource_manager.reload_directories()) // Load the resources from the directories
	{
		logger::error("Resources failed to load");
		return (mIs_ready = false, false);
	}

	logger::info("Resources loaded");

	if (!mScene.load_settings(settings))
		return (mIs_ready = false, false);

	mIs_ready = true;

	mFlags.clean();

	mScene.clean(true);
	mScene.load_scene(settings.get_start_scene());
	return true;
}

bool game::stop()
{
	mScene.clean();
	mFlags.clean();
	return false;
}

void game::clear_scene()
{
	mScene.clean(true);
}

bool game::tick()
{
	if (!mIs_ready || !mScene.is_ready())
		return false;

	mControls.update(*get_renderer());

	engine::renderer& renderer = *get_renderer();

	mScene.tick(mControls);

	mScript.tick();

	if (mScene_load_request.is_requested())
	{
		switch (mScene_load_request.get_player_position_type())
		{
		case scene_load_request::to_position::door:
			mScene.load_scene(mScene_load_request.get_scene_name()
				, mScene_load_request.get_player_door());
			break;

		case scene_load_request::to_position::position:
			mScene.load_scene(mScene_load_request.get_scene_name());
			mScene.get_player().set_position(mScene_load_request.get_player_position());
			break;

		case scene_load_request::to_position::none:
			mScene.load_scene(mScene_load_request.get_scene_name());
			break;
		}
		mScene_load_request.complete();
	}
	return mExit;
}

bool game::restart_game()
{
	logger::info("Reloading entire game...");

	mSave_system.clean();

	bool succ = load(mData_directory);

	logger::info("Game reloaded");
	return succ;
}

void
game::refresh_renderer(engine::renderer & pR)
{
	mScene.set_renderer(pR);
}

// ##########
// script_function
// ##########

script_function::script_function()
{
}

script_function::~script_function()
{
}

bool
script_function::is_running()
{
	if (!mFunc_ctx || !mFunc_ctx->context)
		return false;
	if (mFunc_ctx->context->GetState() == AS::asEXECUTION_FINISHED)
		return false;
	return true;
}

void
script_function::set_function(AS::asIScriptFunction* pFunction)
{
	mFunction = pFunction;
}

util::optional_pointer<AS::asIScriptFunction> rpg::script_function::get_function() const
{
	return mFunction;
}

void
script_function::set_script_system(script_system& pScript_system)
{
	mScript_system = &pScript_system;
}

void
script_function::set_arg(unsigned int index, void* ptr)
{
	assert(mFunc_ctx != nullptr && mFunc_ctx->context != nullptr);
	if(index < mFunction->GetParamCount())
		mFunc_ctx->context->SetArgObject(index, ptr);
}

bool
script_function::call()
{
	if (!is_running())
	{
		return_context();
		mFunc_ctx = mScript_system->create_thread(mFunction, true);
		return true;
	}
	return false;
}

void script_function::return_context()
{
	if (mFunc_ctx && mFunc_ctx->context)
	{
		mFunc_ctx->context->Abort();
		mScript_system->return_context(mFunc_ctx->context);
		mFunc_ctx->context = nullptr;
	}
}


// ##########
// background_music
// ##########

background_music::background_music()
{
	mStream.reset(new engine::sound);
	mOverlap_stream.reset(new engine::sound);
}

void background_music::load_script_interface(script_system & pScript)
{
	pScript.add_function("_music_play", &engine::sound::play, mStream.get());
	pScript.add_function("_music_stop", &engine::sound::stop, mStream.get());
	pScript.add_function("_music_pause", &engine::sound::pause, mStream.get());
	pScript.add_function("_music_get_position", &engine::sound::get_playoffset, mStream.get());
	pScript.add_function("_music_set_position", &engine::sound::set_playoffset, mStream.get());
	pScript.add_function("_music_get_volume", &engine::sound::get_volume, mStream.get());
	pScript.add_function("_music_set_volume", &engine::sound::set_volume, mStream.get());
	pScript.add_function("_music_set_loop", &engine::sound::set_loop, mStream.get());
	pScript.add_function("_music_open", &background_music::script_music_open, this);
	pScript.add_function("_music_is_playing", &engine::sound::is_playing, mStream.get());
	pScript.add_function("_music_get_duration", &engine::sound::get_duration, mStream.get());
	pScript.add_function("_music_swap", &background_music::script_music_swap, this);
	pScript.add_function("_music_start_transition_play", &background_music::script_music_start_transition_play, this);
	pScript.add_function("_music_stop_transition_play", &background_music::script_music_stop_transition_play, this);
	pScript.add_function("_music_set_second_volume", &background_music::script_music_set_second_volume, this);
}

void background_music::clean()
{
	mStream->stop();
	mStream->set_volume(100);
	mOverlap_stream->stop();
	mPath.clear();
	mOverlay_path.clear();
}

void background_music::set_resource_manager(engine::resource_manager & pResource_manager)
{
	mResource_manager = &pResource_manager;
}

void background_music::pause_music()
{
	mStream->pause();
}

bool background_music::script_music_open(const std::string & pName)
{
	auto resource = mResource_manager->get_resource<engine::sound_file>(engine::resource_type::audio, pName);
	if (!resource)
	{
		logger::error("Music '" + pName + "' failed to load");
		return false;
	}
	mStream->set_sound_resource(resource);
	mPath = pName;
	return true;
}

bool background_music::script_music_swap(const std::string & pName)
{
	if (mPath == pName)
		return true;

	if (!mStream->is_playing())
		return script_music_open(pName);

	// Open a second stream and set its position similar to
	// the first one.
	auto resource = mResource_manager->get_resource<engine::sound_file>(engine::resource_type::audio, pName);
	if (!resource)
		return false;
	mOverlap_stream->set_sound_resource(resource);
	mOverlap_stream->set_loop(true);
	mOverlap_stream->set_volume(mStream->get_volume());
	mOverlap_stream->play();
	if (mOverlap_stream->get_duration() >= mStream->get_duration())
		mOverlap_stream->set_playoffset(mStream->get_playoffset());

	// Make the new stream the main stream
	mStream->stop();
	mStream.swap(mOverlap_stream);

	mPath = pName;
	return true;
}

int background_music::script_music_start_transition_play(const std::string & pName)
{
	if (mPath == pName)
		return 0;

	auto resource = mResource_manager->get_resource<engine::sound_file>(engine::resource_type::audio, pName);
	if (!resource)
		return false;
	mOverlap_stream->set_sound_resource(resource);
	mOverlap_stream->set_loop(true);
	mOverlap_stream->set_volume(0);
	mOverlap_stream->play();
	mOverlay_path = pName;
	return 0;
}

void background_music::script_music_stop_transition_play()
{
	mStream->stop();
	mStream.swap(mOverlap_stream);
	mPath.swap(mOverlay_path);
}

void background_music::script_music_set_second_volume(float pVolume)
{
	mOverlap_stream->set_volume(pVolume);
}


// ##########
// save_system
// ##########

const char* int_value_type_name = "int";
const char* float_value_type_name = "float";
const char* string_value_type_name = "string";

save_system::save_system()
{
	mEle_root = nullptr;
}

void save_system::clean()
{
	mDocument.Clear();
	mValues.clear();
}

bool save_system::open_save(const std::string& pPath)
{
	clean();
	if (mDocument.LoadFile(pPath.c_str()))
		return false;
	mEle_root = mDocument.RootElement();
	load_values();
	return true;
}

void save_system::load_flags(flag_container& pFlags)
{
	assert(mEle_root != nullptr);
	auto ele_flag = mEle_root->FirstChildElement("flag");
	while (ele_flag)
	{
		pFlags.set_flag(util::safe_string(ele_flag->Attribute("name")));
		ele_flag = ele_flag->NextSiblingElement("flag");
	}
	logger::info("Loaded " + std::to_string(pFlags.get_count()) + " flags");
}

engine::fvector save_system::get_player_position()
{
	assert(mEle_root != nullptr);
	auto ele_player = mEle_root->FirstChildElement("player");
	return{ ele_player->FloatAttribute("x")
		, ele_player->FloatAttribute("y") };
}

std::string save_system::get_scene_path()
{
	assert(mEle_root != nullptr);
	auto ele_scene = mEle_root->FirstChildElement("scene");
	return util::safe_string(ele_scene->Attribute("path"));
}

std::string save_system::get_scene_name()
{
	assert(mEle_root != nullptr);
	auto ele_scene = mEle_root->FirstChildElement("scene");
	return util::safe_string(ele_scene->Attribute("name"));
}

util::optional<int> save_system::get_int_value(const engine::encoded_path & pPath) const
{
	value* val = find_value(pPath);
	auto cast = dynamic_cast<int_value*>(val);
	if (!cast)
		return{};
	return cast->mValue;
}

util::optional<float> save_system::get_float_value(const engine::encoded_path & pPath) const
{
	value* val = find_value(pPath);
	auto cast = dynamic_cast<float_value*>(val);
	if (!cast)
		return{};
	return cast->mValue;
}

util::optional<std::string> save_system::get_string_value(const engine::encoded_path & pPath) const
{
	value* val = find_value(pPath);
	auto cast = dynamic_cast<string_value*>(val);
	if (!cast)
		return{};
	return cast->mValue;
}

void save_system::new_save()
{
	// Create saves folder if it doesn't exist
	if (!engine::fs::exists(defs::DEFAULT_SAVES_PATH))
		engine::fs::create_directory(defs::DEFAULT_SAVES_PATH);

	mDocument.Clear();
	mEle_root = mDocument.NewElement("save_file");
	mDocument.InsertEndChild(mEle_root);
}

void save_system::save(const std::string& pPath)
{
	assert(mEle_root != nullptr);
	save_values();
	mDocument.SaveFile(pPath.c_str());
}

void save_system::save_flags(flag_container& pFlags)
{
	assert(mEle_root != nullptr);
	for (auto &i : pFlags)
	{
		auto ele_flag = mDocument.NewElement("flag");
		ele_flag->SetAttribute("name", i.c_str());
		mEle_root->InsertEndChild(ele_flag);
	}
	logger::info("Saved " + std::to_string(pFlags.get_count()) + " flags");
}

void save_system::save_scene(scene& pScene)
{
	assert(mEle_root != nullptr);
	auto ele_scene = mDocument.NewElement("scene");
	mEle_root->InsertFirstChild(ele_scene);
	ele_scene->SetAttribute("name", pScene.get_name().c_str());
	ele_scene->SetAttribute("path", pScene.get_path().c_str());

	logger::info("Saved scene '" + pScene.get_path() + "'");

	save_player(pScene.get_player());
}

void save_system::save_player(player_character& pPlayer)
{
	assert(mEle_root != nullptr);
	auto ele_scene = mDocument.NewElement("player");
	mEle_root->InsertFirstChild(ele_scene);
	ele_scene->SetAttribute("x", pPlayer.get_position().x);
	ele_scene->SetAttribute("y", pPlayer.get_position().y);

	logger::info("Saved player position at " + pPlayer.get_position().to_string());
}

void save_system::value_factory(tinyxml2::XMLElement * pEle)
{
	std::unique_ptr<value> new_value;

	// Create
	const std::string type = pEle->Attribute("type");
	if (type == int_value_type_name)
		new_value.reset(new int_value);
	else if (type == float_value_type_name)
		new_value.reset(new float_value);
	else if (type == string_value_type_name)
		new_value.reset(new string_value);

	auto ele_path = pEle->FirstChildElement("path");
	auto ele_value = pEle->FirstChildElement("value");

	// Set path and load stuff
	new_value->mPath.parse(ele_path->GetText());
	new_value->load(ele_value);

	mValues.push_back(std::move(new_value));
}

void save_system::load_values()
{
	assert(mEle_root != nullptr);
	auto ele_values = mEle_root->FirstChildElement("values");
	if (!ele_values)
		return;
	auto ele_val = ele_values->FirstChildElement();
	while (ele_val)
	{
		value_factory(ele_val);
		ele_val = ele_val->NextSiblingElement();
	}

	logger::info("Loaded " + std::to_string(mValues.size()) + " values");
}

void save_system::save_values()
{
	assert(mEle_root != nullptr);
	auto ele_values = mDocument.NewElement("values");
	mEle_root->InsertFirstChild(ele_values);
	for (auto& i : mValues)
	{
		if (i->mPath.empty())
		{
			logger::warning("There is a value with no path");
			continue;
		}
		// Create the entry
		auto ele_entry = mDocument.NewElement("entry");
		ele_values->InsertEndChild(ele_entry);

		// Add the name
		auto ele_path = mDocument.NewElement("path");
		ele_path->SetText(i->mPath.string().c_str());
		ele_entry->InsertFirstChild(ele_path);

		// Add the value data
		auto ele_value = mDocument.NewElement("value");
		ele_entry->InsertEndChild(ele_value);

		i->save(ele_entry, ele_value);
	}

	logger::info("Saved " + std::to_string(mValues.size()) + " values");
}

std::vector<std::string> save_system::get_directory_entries(const engine::encoded_path & pDirectory) const
{
	std::vector<std::string> ret;
	for (auto& i : mValues)
	{
		if (i->mPath.in_directory(pDirectory))
		{
			const std::string entry_path = i->mPath.get_section(pDirectory.get_sub_length());

			// Check if this value already exists
			bool already_has_entry = false;
			for (auto& j : ret)
				if (j == entry_path)
				{
					already_has_entry = true;
					break;
				}
			if (already_has_entry)
				continue;

			ret.push_back(entry_path);
		}
	}
	return ret;
}

bool save_system::set_value(const engine::encoded_path & pPath, int pValue)
{
	auto val = ensure_existence<int_value>(pPath);
	if (!val)
		return false;
	val->mValue = pValue;
	return true;
}

bool save_system::set_value(const engine::encoded_path & pPath, float pValue)
{
	auto val = ensure_existence<float_value>(pPath);
	if (!val)
		return false;
	val->mValue = pValue;
	return true;
}

bool save_system::set_value(const engine::encoded_path & pPath, const std::string & pValue)
{
	auto val = ensure_existence<string_value>(pPath);
	if (!val)
		return false;
	val->mValue = pValue;
	return true;
}

bool save_system::remove_value(const engine::encoded_path & pPath)
{
	for (size_t i = 0; i < mValues.size(); i++)
	{
		if (mValues[i]->mPath == pPath)
		{
			mValues.erase(mValues.begin() + i);
			return true;
		}
	}
	return false;
}

bool save_system::has_value(const engine::encoded_path & pPath) const
{
	return find_value(pPath) != nullptr;
}

save_system::value* save_system::find_value(const engine::encoded_path& pPath) const
{
	for (auto& i : mValues)
		if (i->mPath == pPath)
			return i.get();
	return nullptr;
}

void save_system::int_value::save(tinyxml2::XMLElement * pEle, tinyxml2::XMLElement * pEle_value) const
{
	pEle->SetAttribute("type", int_value_type_name);
	pEle_value->SetText(mValue);
}

void save_system::int_value::load(tinyxml2::XMLElement * pEle_value)
{
	mValue = pEle_value->IntText();
}

void save_system::float_value::save(tinyxml2::XMLElement * pEle, tinyxml2::XMLElement * pEle_value) const
{
	pEle->SetAttribute("type", float_value_type_name);
	pEle_value->SetText(mValue);
}

void save_system::float_value::load(tinyxml2::XMLElement * pEle_value)
{
	mValue = pEle_value->FloatText();
}

void save_system::string_value::save(tinyxml2::XMLElement * pEle, tinyxml2::XMLElement * pEle_value) const
{
	pEle->SetAttribute("type", string_value_type_name);
	pEle_value->SetText(mValue.c_str());
}

void save_system::string_value::load(tinyxml2::XMLElement * pEle_value)
{
	mValue = util::safe_string(pEle_value->GetText());
}

// ##########
// colored_overlay
// ##########

colored_overlay::colored_overlay()
{
	clean();
	mOverlay.set_depth(-10000);
}

void colored_overlay::load_script_interface(script_system & pScript)
{
	pScript.add_function("set_overlay_color", &colored_overlay::script_set_overlay_color, this);
	pScript.add_function("set_overlay_opacity", &colored_overlay::script_set_overlay_opacity, this);
}

void colored_overlay::clean()
{
	mOverlay.set_color({ 0, 0, 0, 0 });
}

void colored_overlay::refresh_renderer(engine::renderer& pR)
{
	pR.add_object(mOverlay);
	mOverlay.set_size({1000, 1000});
}

void colored_overlay::script_set_overlay_color(int r, int g, int b)
{
	auto color = mOverlay.get_color();
	mOverlay.set_color(engine::color(
		util::clamp(r, 0, 255)
		, util::clamp(g, 0, 255)
		, util::clamp(b, 0, 255)
		, color.a ));
}

void colored_overlay::script_set_overlay_opacity(int a)
{
	auto color = mOverlay.get_color();
	color.a = util::clamp(a, 0, 255);
	mOverlay.set_color(color);
}

// ##########
// pathfinding_system
// ##########

pathfinding_system::pathfinding_system()
{
	mPathfinder.set_collision_callback(
		[&](engine::fvector& pos) ->bool
	{
		auto hit = mCollision_system->get_container().first_collision(collision_box::type::wall, { pos, { 0.9f, 0.9f } });
		return (bool)hit;
	});
}

void pathfinding_system::set_collision_system(collision_system & pCollision_system)
{
	mCollision_system = &pCollision_system;
}

void pathfinding_system::load_script_interface(script_system & pScript)
{	
	pScript.add_function("find_path", &pathfinding_system::script_find_path, this);
	pScript.add_function("find_path_partial", &pathfinding_system::script_find_path_partial, this);
}

bool pathfinding_system::script_find_path(AS_array<engine::fvector>& pScript_path, engine::fvector pStart, engine::fvector pDestination)
{
	mPathfinder.set_path_limit(1000);

	if (mPathfinder.start(pStart, pDestination))
	{
		auto path = mPathfinder.construct_path();
		for (auto& i : path)
			pScript_path.InsertLast(&i);
		return true;
	}
	return false;
}

bool pathfinding_system::script_find_path_partial(AS_array<engine::fvector>& pScript_path, engine::fvector pStart, engine::fvector pDestination, int pCount)
{
	mPathfinder.set_path_limit(pCount);

	bool retval = mPathfinder.start(pStart, pDestination);

	auto path = mPathfinder.construct_path();
	for (auto& i : path)
		pScript_path.InsertLast(&i);
	return retval;
}

// #########
// dialog_text_entity
// #########

dialog_text_entity::dialog_text_entity()
{
	set_interval(defs::DEFAULT_DIALOG_SPEED);
	mMax_lines = 0;
	mWord_wrap = 0;
}

void dialog_text_entity::clear()
{
	mRevealing = false;
	mText.set_text("");
}


int dialog_text_entity::draw(engine::renderer & pR)
{
	mNew_character = false;
	if (mRevealing)
		do_reveal();

	return text_entity::draw(pR);
}

bool dialog_text_entity::is_revealing()
{
	return mRevealing;
}

void dialog_text_entity::reveal(const std::string & pText, bool pAppend)
{
	if (pText.empty())
		return;

	mTimer.start();
	mRevealing = true;

	if (pAppend)
	{
		mFull_text += pText;
	}
	else
	{
		mFull_text = pText;
		mText.set_text("");
		mCount = 0;
	}
	adjust_text();
}

void dialog_text_entity::skip_reveal()
{
	if (mRevealing)
	{
		mCount = mFull_text.length();
	}
}

void dialog_text_entity::set_interval(float pMilliseconds)
{
	mTimer.set_interval(pMilliseconds*0.001f);
}

bool dialog_text_entity::has_revealed_character()
{
	return mNew_character;
}

void dialog_text_entity::set_wordwrap(size_t pLength)
{
	mWord_wrap = pLength;
	adjust_text();
}

void dialog_text_entity::set_max_lines(size_t pLines)
{
	mMax_lines = pLines;
	adjust_text();
}

void dialog_text_entity::adjust_text()
{
	mFull_text.word_wrap(mWord_wrap);
}

void dialog_text_entity::do_reveal()
{
	size_t iterations = mTimer.get_count();
	if (iterations > 0)
	{
		mCount += iterations;
		mCount = util::clamp<size_t>(mCount, 0, mFull_text.length());

		engine::text_format cut_text = mFull_text.substr(0, mCount);

		// Remove lines when there are too many
		if (mMax_lines > 0)
		{
			cut_text.limit_lines(mMax_lines);
		}

		mText.set_text(cut_text);

		if (mCount == mFull_text.length())
			mRevealing = false;

		mNew_character = true;

		mTimer.start();
	}
}

text_entity::text_entity()
{
	set_dynamic_depth(false);
}

int text_entity::draw(engine::renderer & pR)
{
	update_depth();
	mText.set_unit(get_unit());
	mText.set_position(calculate_draw_position());
	mText.draw(pR);
	return 0;
}

bool game_settings_loader::load(const std::string & pPath, const std::string& pPrefix_path)
{
	using namespace tinyxml2;
	XMLDocument doc;

	if (doc.LoadFile(pPath.c_str()))
	{
		logger::error("Could not load game file at '" + pPath + "'");
		return false;
	}
	return parse_settings(doc, pPrefix_path);
}

bool game_settings_loader::load_memory(const char * pData, size_t pSize, const std::string& pPrefix_path)
{
	using namespace tinyxml2;
	XMLDocument doc;

	if (doc.Parse(pData, pSize))
	{
		logger::error("Could not load game file");
		return false;
	}
	return parse_settings(doc, pPrefix_path);
}

const std::string & game_settings_loader::get_start_scene() const
{
	return mStart_scene;
}

const std::string & game_settings_loader::get_player_texture() const
{
	return mPlayer_texture;
}

engine::fvector game_settings_loader::get_screen_size() const
{
	return mScreen_size;
}

float game_settings_loader::get_unit_pixels() const
{
	return pUnit_pixels;
}

const engine::controls& game_settings_loader::get_key_bindings() const
{
	return mKey_bindings;
}

bool game_settings_loader::parse_settings(tinyxml2::XMLDocument & pDoc, const std::string& pPrefix_path)
{
	auto ele_root = pDoc.RootElement();

	if (!ele_root)
	{
		logger::error("Root element missing in settings");
		return false;
	}

	auto ele_scene = ele_root->FirstChildElement("scene");
	if (!ele_scene || !ele_scene->Attribute("name"))
	{
		logger::error("Please specify the scene to start with");
		return false;
	}
	mStart_scene = util::safe_string(ele_scene->Attribute("name"));

	auto ele_player = ele_root->FirstChildElement("player");
	if (!ele_player || !ele_player->Attribute("texture"))
	{
		logger::error("Please specify the player texture");
		return false;
	}
	mPlayer_texture = util::safe_string(ele_player->Attribute("texture"));

	auto ele_tile_size = ele_root->FirstChildElement("tile_size");
	if (!ele_tile_size || !ele_tile_size->Attribute("pixels"))
	{
		logger::error("Please specify the pixel size of the tiles");
		return false;
	}
	pUnit_pixels = ele_tile_size->FloatAttribute("pixels");

	auto ele_screen_size = ele_root->FirstChildElement("screen_size");
	if (!ele_screen_size)
	{
		logger::error("Please specify the screen size");
		return false;
	}
	mScreen_size = util::shortcuts::load_vector_float_att(ele_screen_size);
	
	auto ele_controls = ele_root->FirstChildElement("controls");
	if (!ele_controls)
	{
		logger::info("No controls specified");
		return false;
	}
	else
		parse_key_bindings(ele_controls);

	return true;
}

bool game_settings_loader::parse_key_bindings(tinyxml2::XMLElement * pEle)
{
	mKey_bindings.clean();

	auto current_entry = pEle->FirstChildElement();
	while (current_entry)
	{
		const std::string name = util::safe_string(current_entry->Name());
		parse_binding_attributes(current_entry, name, "key", false);
		parse_binding_attributes(current_entry, name, "alt", true);
		mKey_bindings.set_press_only(name, current_entry->BoolAttribute("press"));
		current_entry = current_entry->NextSiblingElement();
	}
	return true;
}

bool game_settings_loader::parse_binding_attributes(tinyxml2::XMLElement * pEle, const std::string& pName, const std::string& pPrefix, bool pAlternative)
{
	size_t i = 0;
	while (auto key = pEle->Attribute((pPrefix + std::to_string(i)).c_str()))
	{
		if (!mKey_bindings.bind_key(pName, key, pAlternative))
		{
			logger::warning("Failed to bind key '" + std::string(key) + "' with '" + pName + "'");
			break;
		}
		++i;
	}
	return true;
}

scene_load_request::scene_load_request()
{
	mRequested = false;
	mTo_position = to_position::none;
}

void scene_load_request::request_load(const std::string & pScene_name)
{
	mRequested = true;
	mScene_name = pScene_name;
}

bool scene_load_request::is_requested() const
{
	return mRequested;
}

const std::string & rpg::scene_load_request::get_scene_name() const
{
	return mScene_name;
}

void scene_load_request::complete()
{
	mRequested = false;
	mTo_position = to_position::none;
	mScene_name.clear();
}

void scene_load_request::set_player_position(engine::fvector pPosition)
{
	mTo_position = to_position::position;
	mPosition = pPosition;
}

void scene_load_request::set_player_position(const std::string & pDoor)
{
	mTo_position = to_position::door;
	mDoor = pDoor;
}

scene_load_request::to_position scene_load_request::get_player_position_type() const
{
	return mTo_position;
}

const std::string & scene_load_request::get_player_door() const
{
	return mDoor;
}

engine::fvector scene_load_request::get_player_position() const
{
	return mPosition;
}

scenes_directory::scenes_directory()
{
	mPath = defs::DEFAULT_SCENES_PATH.string();
}

bool scenes_directory::load(engine::resource_manager & pResource_manager)
{
	if (!engine::fs::exists(mPath))
	{
		logger::error("Scenes directory does not exist");
		return false;
	}

	for (const auto& i : engine::fs::recursive_directory_iterator(mPath))
	{
		const auto& script_path = i.path();

		if (script_path.extension().string() == ".xml")
		{
			std::shared_ptr<scene_script_context> context(new scene_script_context);
			//context->set_path(script_path.parent_path() + (script_path.stem().string() + ".as"));
			//pResource_manager.add_resource(engine::resource_type::script, , context);
		}
	}
	return true;
}

/* a doodle:
some arbitrary string comparison

double rate_string_comparison(const std::string& pStr1, const std::string& pStr2)
{
double final_rating = 0;

if (pStr1.length() != pStr2.length())
final_rating = std::abs(static_cast<double>(pStr1.length()) - static_cast<double>(pStr2.length()));

if (pStr1.length() <= (pStr2.length() / 2))
{
for (auto i = pStr2.begin(); i != pStr2.end() - pStr1.length(); i++)
{
if (std::string(i, i + pStr1.length()) == pStr1)
final_rating /= 2;
}
}

return final_rating;
}*/

void scenes_directory::set_path(const std::string & pFilepath)
{
	mPath = pFilepath;
}

void scenes_directory::set_script_system(script_system & pScript)
{
	mScript = &pScript;
}

#ifndef LOCKED_RELEASE_MODE
terminal_gui::terminal_gui()
{
	mEb_input = std::make_shared<tgui::EditBox>();
	mEb_input->setSize(400, 25);
	mEb_input->hide();

	mLb_log = std::make_shared<tgui::Label>();
	mLb_log->setTextSize(10);
	mLb_log->setTextColor({ 100, 255, 100, 255 });
	mLb_log->getRenderer()->setBackgroundColor({ 0, 0, 0, 180 });
	mLb_log->hide();

	mLb_autocomplete = std::make_shared<tgui::ListBox>();
	mLb_autocomplete->setTextSize(10);
	mLb_autocomplete->hide();
}

void terminal_gui::set_terminal_system(engine::terminal_system & pTerminal_system)
{
	mTerminal_system = &pTerminal_system;
	mEb_input->connect("TextChanged",
		[&]()
	{
		refresh_autocomplete();
	});
	mEb_input->connect("ReturnKeyPressed",
		[&](sf::String pText)
	{
		if (!pTerminal_system.execute(pText))
		{
			logger::error("Command failed '" + std::string(pText) + "'");
		}
		mEb_input->setText("");
		if (mHistory.empty() || mHistory.back() != pText)
			mHistory.push_back(std::string(pText));
		mCurrent_history_entry = mHistory.size();
		refresh_log();
		mLb_autocomplete->hide();
	});

	mLb_autocomplete->connect("itemselected", [&](sf::String pItem)
	{
		mEb_input->setText(pItem);
		mLb_autocomplete->hide();
		mEb_input->focus();
	});
}

void terminal_gui::load_gui(engine::renderer & pR)
{
	mEb_input->setPosition("&.width - width", "&.height - height");
	pR.get_tgui().add(mEb_input);

	mLb_log->setPosition(tgui::bindLeft(mEb_input), 0);
	mLb_log->setSize(tgui::bindWidth(mEb_input), tgui::bindTop(mEb_input));
	pR.get_tgui().add(mLb_log);

	mLb_autocomplete->setPosition(tgui::bindLeft(mEb_input), tgui::bindTop(mEb_input) - 200);
	mLb_autocomplete->setSize(tgui::bindWidth(mEb_input), 200);
	pR.get_tgui().add(mLb_autocomplete);
}

inline std::string snip_bottom_string(const std::string& pStr, size_t pLines)
{
	size_t lines = 0;
	size_t i = 1;
	for (; i < pStr.size(); i++)
	{
		if (pStr[pStr.size() - i] == '\n')
			++lines;

		if (lines == pLines)
			break;
	}
	return std::string(pStr.end() - i, pStr.end());
}

void terminal_gui::update(engine::renderer& pR)
{
	if (pR.is_key_down(engine::renderer::key_type::LControl, true) // Toggle visibility
		&& pR.is_key_pressed(engine::renderer::key_type::T, true))
	{
		if (mEb_input->isVisible())
		{
			mEb_input->hide();
			mLb_log->hide();
			mLb_autocomplete->hide();
		}
		else
		{
			mEb_input->show();
			mLb_log->show();
			mEb_input->focus();
		}
		refresh_log();
	}

	if (mEb_input->isFocused())
	{
		if (pR.is_key_pressed(engine::renderer::key_type::Up, true)
			&& mCurrent_history_entry >= 1)
		{
			--mCurrent_history_entry;
			mEb_input->setText(mHistory[mCurrent_history_entry]);
		}

		if (pR.is_key_pressed(engine::renderer::key_type::Down, true)
			&& mCurrent_history_entry < mHistory.size())
		{
			++mCurrent_history_entry;
			if (mCurrent_history_entry == mHistory.size())
				mEb_input->setText("");
			else
				mEb_input->setText(mHistory[mCurrent_history_entry]);
		}
	}

	if (mRefresh_timer.is_reached())
		refresh_log();
}

void terminal_gui::refresh_autocomplete()
{
	mLb_autocomplete->removeAllItems();
	auto hits = mTerminal_system->autocomplete(mEb_input->getText());
	if (hits.empty())
	{
		mLb_autocomplete->hide();
		return;
	}
	mLb_autocomplete->show();

	for (auto& i : hits)
		mLb_autocomplete->addItem(i);
}

void terminal_gui::refresh_log()
{
	if (!mLb_log->isVisible())
		return;
	mRefresh_timer.start(0.1f);

	// Estimate
	const size_t lines = (size_t)(mLb_log->getSize().y / ((float)mLb_log->getTextSize() + 10));
	const std::string log = snip_bottom_string(logger::get_log(), lines);

	if (log != mLb_log->getText())
		mLb_log->setText(log);
}
#endif
