#include <algorithm>
#include <fstream>

#include <engine/logger.hpp>
#include <engine/resource_pack.hpp>
#include <engine/filesystem.hpp>

#include <rpg/rpg.hpp>

using namespace rpg;

// #########
// script_context
// #########

static int add_section_from_pack(const engine::generic_path& pPath, engine::resource_pack& pPack,  AS::CScriptBuilder& pBuilder)
{
	auto data = pPack.read_all(pPath);
	if (data.empty())
		return -1;
	return pBuilder.AddSectionFromMemory(pPath.string().c_str(), &data[0], data.size());
}

static int pack_include_callback(const char *include, const char *from, AS::CScriptBuilder *pBuilder, void *pUser)
{
	engine::resource_pack* pack = reinterpret_cast<engine::resource_pack*>(pUser);
	auto path = engine::generic_path(from).parent() / engine::generic_path(include);
	return add_section_from_pack(path, *pack, *pBuilder);
}

scene_script_context::scene_script_context() :
	mScene_module(nullptr)
{}

scene_script_context::~scene_script_context()
{
}

// TO BE IMPLEMENTED AS A RESOURCE
//void scene_script_context::set_path(const std::string & pFilepath)
//{
//	mScript_path = pFilepath;
//}

//bool scene_script_context::load()
//{
//	assert(!mScript_path.empty());
//	return build_script(mScript_path);
//}

//bool scene_script_context::unload()
//{
//	clean();
//	return true;
//}

void scene_script_context::set_script_system(script_system & pScript)
{
	mScript = &pScript;
}

bool scene_script_context::build_script(const std::string & pPath, const engine::fs::path& pData_path)
{
	logger::info("Compiling script '" + pPath + "'...");

	clean();

	mBuilder.SetIncludeCallback(nullptr, nullptr);

	mBuilder.StartNewModule(mScript->mEngine, pPath.c_str());

	std::string internal_include = "#include \"" + (pData_path / defs::INTERNAL_SCRIPTS_PATH).string() + "\"";
	mBuilder.AddSectionFromMemory("__Internal__", internal_include.c_str());
	
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

bool scene_script_context::build_script(const std::string & pPath, engine::resource_pack & pPack)
{
	logger::info("Compiling script '" + pPath + "'...");

	clean();

	mBuilder.SetIncludeCallback(pack_include_callback, &pPack);
	mBuilder.StartNewModule(mScript->mEngine, pPath.c_str());

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
	return true;
}

std::string scene_script_context::get_metadata_type(const std::string & pMetadata)
{
	for (auto i = pMetadata.begin(); i != pMetadata.end(); i++)
		if (std::isspace(*i))
			return std::string(pMetadata.begin(), i);
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

void scene_script_context::call_all_with_tag(const std::string & pTag)
{
	logger::info("Calling all functions with tag '" + pTag + "'...");

	auto funcs = get_all_with_tag(pTag);
	for (auto& i : funcs)
		i->call();
}

std::vector<std::shared_ptr<script_function>> scene_script_context::get_all_with_tag(const std::string & pTag)
{
	std::vector<std::shared_ptr<script_function>> ret;
	size_t func_count = mScene_module->GetFunctionCount();
	for (size_t i = 0; i < func_count; i++)
	{
		auto func = mScene_module->GetFunctionByIndex(i);
		std::string metadata = util::remove_trailing_whitespace(mBuilder.GetMetadataStringForFunc(func));
		if (metadata == pTag)
		{
			std::shared_ptr<script_function> sfunc(new script_function);
			sfunc->mFunction = func;
			sfunc->mScript_system = mScript;
			ret.push_back(sfunc);
		}
	}
	return ret;
}

void scene_script_context::clear_globals()
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
		const std::string metadata = util::remove_trailing_whitespace(mBuilder.GetMetadataStringForFunc(as_function));
		const std::string type = get_metadata_type(metadata);

		if (type == "group")
		{
			if (metadata == type) // There is no specified group name
			{
				logger::warning("Group name is not specified in function '" + std::string(as_function->GetDeclaration()) + "'");
				continue;
			}

			std::shared_ptr<script_function> function(new script_function);
			function->mScript_system = mScript;
			function->mFunction = as_function;

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

	mResource_manager.add_loader(std::make_shared<texture_loader>());
	mResource_manager.add_loader(std::make_shared<font_loader>());
	mResource_manager.add_loader(std::make_shared<audio_loader>());
}

game::~game()
{
	logger::info("Destroying game");
	mScene.clean();
}

engine::fs::path game::get_slot_path(unsigned int pSlot)
{
	return defs::DEFAULT_SAVES_PATH / ("slot_" + std::to_string(pSlot) + ".xml");
}

void game::save_game()
{
	const std::string path = get_slot_path(mSlot).string();

	logger::info("Saving game...");

	mSave_system.new_save();

	mSave_system.save_values(mValues);
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
	mValues.clear();
	mSave_system.load_values(mValues);
	if (mScript.is_executing())
	{
		mValues.set_value("_nosave/scene_load/request", true);
	}
	else
	{
		mScene.load_scene(mSave_system.get_scene_path());
	}

	logger::info("Loaded " + std::to_string(mFlags.get_count()) + " flag(s)");
	logger::info("Game opened from '" + path + "'");
}

bool game::is_slot_used(unsigned int pSlot)
{
	const std::string path = get_slot_path(pSlot).string();
	std::ifstream stream(path.c_str());
	return stream.good();
}

void game::set_slot(unsigned int pSlot)
{
	mSlot = pSlot;
}

unsigned int game::get_slot()
{
	return mSlot;
}

void game::abort_game()
{
	mExit = true;
}

int game::script_get_int_value(const std::string & pPath) const
{
	auto val = mValues.get_int_value(pPath);
	if (!val)
	{
		logger::warning("Value '" + pPath + "' does not exist");
		return 0;
	}
	return *val;
}

float game::script_get_float_value(const std::string & pPath) const
{
	auto val = mValues.get_float_value(pPath);
	if (!val)
	{
		logger::warning("Value '" + pPath + "' does not exist");
		return 0;
	}
	return *val;
}

bool game::script_get_bool_value(const std::string & pPath) const
{
	auto val = mValues.get_bool_value(pPath);
	if (!val)
	{
		logger::warning("Value '" + pPath + "' does not exist");
		return{};
	}
	return *val;
}

std::string game::script_get_string_value(const std::string & pPath) const
{
	auto val = mValues.get_string_value(pPath);
	if (!val)
	{
		logger::warning("Value '" + pPath + "' does not exist");
		return{};
	}
	return *val;
}

bool game::script_set_int_value(const std::string & pPath, int pValue)
{
	return mValues.set_value(pPath, pValue);
}

bool game::script_set_float_value(const std::string & pPath, float pValue)
{
	return mValues.set_value(pPath, pValue);
}

bool game::script_set_bool_value(const std::string & pPath, bool pValue)
{
	return mValues.set_value(pPath, pValue);
}

bool game::script_set_string_value(const std::string & pPath, const std::string & pValue)
{
	return mValues.set_value(pPath, pValue);
}

AS_array<std::string>* game::script_get_director_entries(const std::string & pPath)
{
	const auto entries = mValues.get_directory_entries(pPath);
	auto arr = mScript.create_array<std::string>(entries.size());
	for (size_t i = 0; i < entries.size(); i++)
		arr->SetValue(i, (void*)&entries[i]);

	return arr;
}

bool game::script_remove_value(const std::string & pPath)
{
	return mValues.remove_value(pPath);
}

bool game::script_has_value(const std::string & pPath)
{
	return mValues.has_value(pPath);
}

float game::script_get_tile_size()
{
	return mScene.get_world_node().get_unit();
}

void game::load_script_interface()
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

	mScript.begin_namespace("values");
	mScript.add_function("get_int", &game::script_get_int_value, this);
	mScript.add_function("get_float", &game::script_get_float_value, this);
	mScript.add_function("get_bool", &game::script_get_bool_value, this);
	mScript.add_function("get_string", &game::script_get_string_value, this);
	mScript.add_function("set", &game::script_set_int_value, this);
	mScript.add_function("set", &game::script_set_float_value, this);
	mScript.add_function("set", &game::script_set_bool_value, this);
	mScript.add_function("set", &game::script_set_string_value, this);
	mScript.add_function("get_entries", &game::script_get_director_entries, this);
	mScript.add_function("remove", &game::script_remove_value, this);
	mScript.add_function("exists", &game::script_has_value, this);
	mScript.end_namespace();

	mScript.add_function("get_tile_size", &game::script_get_tile_size, this);

	mFlags.load_script_interface(mScript);
	mScene.load_script_interface(mScript);
	logger::info("Script interface loaded");
}

void game::load_icon()
{
	if (!get_renderer()->get_window())
		return;
	get_renderer()->get_window()->set_icon((mData_path / "icon.png").string());
}

void game::load_icon_pack()
{
	auto data = mPack.read_all("icon.png");
	get_renderer()->get_window()->set_icon(data);
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
		mData_path = pArgs[0].get_raw();
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
			slot = static_cast<size_t>(std::max(util::to_numeral<int>(pArgs[0].get_raw()), 0));
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
			slot = static_cast<size_t>(std::max(util::to_numeral<int>(pArgs[0].get_raw()), 0));
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
#endif

scene & game::get_scene()
{
	return mScene;
}
engine::resource_manager & game::get_resource_manager()
{
	return mResource_manager;
}
const engine::fs::path & game::get_source_path() const
{
	return mData_path;
}

script_system& game::get_script_system()
{
	return mScript;
}

std::vector<std::string> game::get_scene_list() const
{
	std::vector<std::string> ret;

	const engine::generic_path dir = (mData_path / defs::DEFAULT_SCENES_PATH).string();
	for (auto& i : engine::fs::recursive_directory_iterator(mData_path / defs::DEFAULT_SCENES_PATH))
	{
		engine::generic_path path = i.path().string();
		if (path.extension() == ".xml")
		{
			path.snip_path(dir);
			path.remove_extension();
			ret.push_back(path.string());
		}
	}

	std::sort(ret.begin(), ret.end());
	return ret;
}

float game::get_delta()
{
	return get_renderer()->get_delta();
}

bool game::load(engine::fs::path pData_path)
{
	using namespace tinyxml2;
	
	mData_path = pData_path;
	mScene.set_data_source(mData_path);

	game_settings_loader settings;
	if (engine::fs::is_directory(pData_path)) // Data folder 
	{
		if (!load_directory(settings))
			return false;
	}
	else // This is a package
	{
		if (!load_pack(settings))
			return false;
	}

	logger::info("Settings loaded");

	if (get_renderer()->get_window())
		get_renderer()->get_window()->set_title(settings.get_title());
	
	get_renderer()->set_target_size(settings.get_screen_size());

	mControls = settings.get_key_bindings();

	logger::info("Loading Resources...");

	if (!mResource_manager.reload_all()) // Load the resources from the loaders
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

bool game::create_scene(const std::string & pName, const std::string& pTexture)
{
	const auto xml_path = mData_path / defs::DEFAULT_SCENES_PATH / (pName + ".xml");
	const auto script_path = mData_path / defs::DEFAULT_SCENES_PATH / (pName + ".as");

	if (engine::fs::exists(xml_path) || engine::fs::exists(script_path))
	{
		logger::error("Scene '" + pName + "' already exists");
		return false;
	}

	std::string xml_default_str;
	if (!util::text_file_readall("./editor/scene-defaults/default-xml.xml", xml_default_str))
	{
		logger::error("Failed to load default .xml file for new scene");
		return false;
	}

	util::replace_all_with(xml_default_str, "%texture%", pTexture);

	std::string as_default_str;
	if (!util::text_file_readall("./editor/scene-defaults/default-as.as", as_default_str))
	{
		logger::error("Failed to load default .as file for new scene");
		return false;
	}

	// Ensure the existance of the directories
	engine::fs::create_directories(xml_path.parent_path());

	// Create xml file
	std::ofstream xml(xml_path.string().c_str());
	xml << xml_default_str;
	xml.close();

	// Create script file
	std::ofstream script(script_path.string().c_str());
	script << as_default_str;
	script.close();

	return true;
}

bool game::tick()
{
	if (!mIs_ready || !mScene.is_ready())
		return false;
	mControls.update(*get_renderer());
	engine::renderer& renderer = *get_renderer();
	mScript.tick();

	auto load_scene_request = mValues.get_bool_value("_nosave/load_scene/request");
	if (load_scene_request && *load_scene_request)
	{
		auto scene_name = mValues.get_string_value("_nosave/load_scene/scene");
		if (scene_name && !scene_name->empty())
		{
			mValues.set_value("_nosave/load_scene/request", false);
			mScene.load_scene(*scene_name);
		}
	}

	return mExit;
}

bool game::restart_game()
{
	logger::info("Reloading entire game...");

	mSave_system.clean();

	bool succ = load(mData_path);

	logger::info("Game reloaded");
	return succ;
}

void game::refresh_renderer(engine::renderer & pR)
{
	mScene.set_renderer(pR);
}

bool game::load_directory(game_settings_loader& pSettings)
{
	logger::info("Loading settings from data folder...");

	const engine::fs::path settings_path = mData_path / "game.xml";
	if (!pSettings.load(settings_path.string(), mData_path.string() + "/"))
		return mIs_ready = false;

	mResource_manager.set_directory(mData_path.string());
	mResource_manager.set_resource_pack(nullptr);
	mScene.set_resource_pack(nullptr);

	load_icon();
	return true;
}

bool game::load_pack(game_settings_loader & pSettings)
{
	logger::info("Loading settings from pack...");

	if (!mPack.open(mData_path.string()))
	{
		logger::error("Could not load pack");
		return mIs_ready = false;
	}

	const auto settings_data = mPack.read_all("game.xml");
	if (!pSettings.load_memory(&settings_data[0], settings_data.size()))
		return mIs_ready = false;

	mResource_manager.set_resource_pack(&mPack);
	mScene.set_resource_pack(&mPack);

	load_icon_pack();
	return false;
}

// ##########
// script_function
// ##########

bool script_function::is_running()
{
	if (!mFunc_ctx || !mFunc_ctx->context)
		return false;
	if (mFunc_ctx->context->GetState() == AS::asEXECUTION_FINISHED)
		return false;
	return true;
}

void script_function::set_arg(unsigned int index, void* ptr)
{
	assert(mFunc_ctx != nullptr);
	assert(mFunc_ctx->context != nullptr);
	assert(index < mFunction->GetParamCount());
	mFunc_ctx->context->SetArgObject(index, ptr);
}

bool script_function::call()
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
		mScript_system->mEngine->ReturnContext(mFunc_ctx->context);
		mFunc_ctx->context = nullptr;
	}
}


// ##########
// background_music
// ##########

background_music::background_music()
{
	mStream.reset(new engine::sound);
	mStream->set_mono(false);
	mOverlap_stream.reset(new engine::sound);
	mOverlap_stream->set_mono(false);
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
	mOverlap_stream->set_volume(100);
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

void background_music::set_mixer(engine::mixer & pMixer)
{
	mStream->attach_mixer(pMixer);
	mOverlap_stream->attach_mixer(pMixer);
}

bool background_music::script_music_open(const std::string & pName)
{
	auto resource = mResource_manager->get_resource<engine::sound_file>("audio", pName);
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
	auto resource = mResource_manager->get_resource<engine::sound_file>("audio", pName);
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

	auto resource = mResource_manager->get_resource<engine::sound_file>("audio", pName);
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

save_system::save_system()
{
	mEle_root = nullptr;
}

void save_system::clean()
{
	mDocument.Clear();
}

bool save_system::open_save(const std::string& pPath)
{
	clean();
	if (mDocument.LoadFile(pPath.c_str()))
		return false;
	mEle_root = mDocument.RootElement();
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

void save_system::load_values(value_container & pContainer)
{
	auto ele_values = mEle_root->FirstChildElement("values");
	pContainer.load_xml_values(ele_values);
}

util::optional<int> value_container::get_int_value(const engine::generic_path& pPath) const
{
	value* val = find_value(pPath);
	auto cast = dynamic_cast<int_value*>(val);
	if (!cast)
		return{};
	return cast->mValue;
}

util::optional<float> value_container::get_float_value(const engine::generic_path& pPath) const
{
	value* val = find_value(pPath);
	auto cast = dynamic_cast<float_value*>(val);
	if (!cast)
		return{};
	return cast->mValue;
}

util::optional<bool> rpg::value_container::get_bool_value(const engine::generic_path & pPath) const
{
	value* val = find_value(pPath);
	auto cast = dynamic_cast<bool_value*>(val);
	if (!cast)
		return{};
	return cast->mValue;
}

util::optional<std::string> value_container::get_string_value(const engine::generic_path& pPath) const
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
	assert(mEle_root);
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
}

void save_system::save_values(const value_container & pContainer)
{
	auto ele_values = mDocument.NewElement("values");
	mEle_root->InsertEndChild(ele_values);
}

const char* int_value_type_name = "int";
const char* float_value_type_name = "float";
const char* bool_value_type_name = "boolean";
const char* string_value_type_name = "string";

void value_container::value_factory(tinyxml2::XMLElement * pEle)
{
	std::unique_ptr<value> new_value;

	// Create
	const std::string type = pEle->Attribute("type");
	if (type == int_value_type_name)
		new_value.reset(new int_value);
	else if (type == float_value_type_name)
		new_value.reset(new float_value);
	else if (type == bool_value_type_name)
		new_value.reset(new bool_value);
	else if (type == string_value_type_name)
		new_value.reset(new string_value);

	auto ele_path = pEle->FirstChildElement("path");
	auto ele_value = pEle->FirstChildElement("value");

	// Set path and load stuff
	new_value->mPath.parse(ele_path->GetText());
	new_value->load(ele_value);

	mValues.push_back(std::move(new_value));
}

void value_container::load_xml_values(tinyxml2::XMLElement * pRoot)
{
	assert(pRoot);
	auto ele_val = pRoot->FirstChildElement();
	while (ele_val)
	{
		value_factory(ele_val);
		ele_val = ele_val->NextSiblingElement();
	}

	logger::info("Loaded " + std::to_string(mValues.size()) + " values");
}

void value_container::save_xml_values(tinyxml2::XMLDocument& pDoc, tinyxml2::XMLElement * pRoot)
{
	assert(pRoot);
	for (auto& i : mValues)
	{
		if (i->mPath.empty())
		{
			logger::warning("There is a value with no path");
			continue;
		}

		if (i->mPath[0] == "_nosave")
			continue;

		// Create the entry
		auto ele_entry = pDoc.NewElement("entry");
		pRoot->InsertEndChild(ele_entry);

		// Add the name
		auto ele_path = pDoc.NewElement("path");
		ele_path->SetText(i->mPath.string().c_str());
		ele_entry->InsertFirstChild(ele_path);

		// Add the value data
		auto ele_value = pDoc.NewElement("value");
		ele_entry->InsertEndChild(ele_value);
		i->save(ele_entry, ele_value);
	}

	logger::info("Saved " + std::to_string(mValues.size()) + " values");
}

void value_container::clear()
{
	mValues.clear();
}

std::vector<std::string> value_container::get_directory_entries(const engine::generic_path & pDirectory) const
{
	std::vector<std::string> ret;
	for (auto& i : mValues)
	{
		if (i->mPath.in_directory(pDirectory))
		{
			const std::string entry_path = i->mPath[pDirectory.get_sub_length()];

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

bool value_container::set_value(const engine::generic_path & pPath, int pValue)
{
	auto val = ensure_existence<int_value>(pPath);
	if (!val)
		return false;
	val->mValue = pValue;
	return true;
}

bool value_container::set_value(const engine::generic_path & pPath, float pValue)
{
	auto val = ensure_existence<float_value>(pPath);
	if (!val)
		return false;
	val->mValue = pValue;
	return true;
}

bool value_container::set_value(const engine::generic_path & pPath, bool pValue)
{
	auto val = ensure_existence<bool_value>(pPath);
	if (!val)
		return false;
	val->mValue = pValue;
	return false;
}

bool value_container::set_value(const engine::generic_path & pPath, const std::string & pValue)
{
	auto val = ensure_existence<string_value>(pPath);
	if (!val)
		return false;
	val->mValue = pValue;
	return true;
}

bool value_container::remove_value(const engine::generic_path & pPath)
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

bool value_container::has_value(const engine::generic_path & pPath) const
{
	return find_value(pPath) != nullptr;
}

value_container::value* value_container::find_value(const engine::generic_path& pPath) const
{
	for (auto& i : mValues)
		if (i->mPath == pPath)
			return i.get();
	return nullptr;
}

void value_container::int_value::save(tinyxml2::XMLElement * pEle, tinyxml2::XMLElement * pEle_value) const
{
	pEle->SetAttribute("type", int_value_type_name);
	pEle_value->SetText(mValue);
}

void value_container::int_value::load(tinyxml2::XMLElement * pEle_value)
{
	mValue = pEle_value->IntText();
}

void value_container::float_value::save(tinyxml2::XMLElement * pEle, tinyxml2::XMLElement * pEle_value) const
{
	pEle->SetAttribute("type", float_value_type_name);
	pEle_value->SetText(mValue);
}

void value_container::float_value::load(tinyxml2::XMLElement * pEle_value)
{
	mValue = pEle_value->FloatText();
}

void value_container::bool_value::save(tinyxml2::XMLElement * pEle, tinyxml2::XMLElement * pEle_value) const
{
	pEle->SetAttribute("type", bool_value_type_name);
	pEle_value->SetText(mValue);
}

void value_container::bool_value::load(tinyxml2::XMLElement * pEle_value)
{
	mValue = pEle_value->BoolText();
}


void value_container::string_value::save(tinyxml2::XMLElement * pEle, tinyxml2::XMLElement * pEle_value) const
{
	pEle->SetAttribute("type", string_value_type_name);
	pEle_value->SetText(mValue.c_str());
}

void value_container::string_value::load(tinyxml2::XMLElement * pEle_value)
{
	mValue = util::safe_string(pEle_value->GetText());
}

// ##########
// colored_overlay
// ##########

colored_overlay::colored_overlay()
{
	reset();
	mOverlay.set_depth(-10000);
}

void colored_overlay::load_script_interface(script_system & pScript)
{
	pScript.add_function("set_overlay_color", &colored_overlay::script_set_overlay_color, this);
	pScript.add_function("set_overlay_opacity", &colored_overlay::script_set_overlay_opacity, this);
}

void colored_overlay::reset()
{
	mOverlay.set_color({ 0, 0, 0, 0 });
}

void colored_overlay::refresh_renderer(engine::renderer& pR)
{
	pR.add_object(mOverlay);
	mOverlay.set_size({1000, 1000});
}

void colored_overlay::script_set_overlay_color(float r, float g, float b)
{
	mOverlay.set_color(mOverlay.get_color().clamp());
}

void colored_overlay::script_set_overlay_opacity(float a)
{
	auto color = mOverlay.get_color();
	color.a = util::clamp<float>(a, 0, 1);
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
	mText.set_internal_parent(*this);
}

int text_entity::draw(engine::renderer & pR)
{
	update_depth();
	mText.set_unit(get_unit());
	mText.set_position(calculate_offset());
	mText.draw(pR);
	return 0;
}

bool game_settings_loader::load(const std::string & pPath, const std::string& pPrefix_path)
{
	tinyxml2::XMLDocument doc;

	if (doc.LoadFile(pPath.c_str()))
	{
		logger::error("Could not load game file at '" + pPath + "'");
		return false;
	}
	return parse_settings(doc, pPrefix_path);
}

bool game_settings_loader::load_memory(const char * pData, size_t pSize, const std::string& pPrefix_path)
{
	tinyxml2::XMLDocument doc;

	if (doc.Parse(pData, pSize))
	{
		logger::error("Could not load game file");
		return false;
	}
	return parse_settings(doc, pPrefix_path);
}

const std::string & game_settings_loader::get_title() const
{
	return mTitle;
}

void game_settings_loader::set_title(const std::string & pTitle)
{
	mTitle = pTitle;
}

const std::string & game_settings_loader::get_start_scene() const
{
	return mStart_scene;
}

void game_settings_loader::set_start_scene(const std::string & pName)
{
	mStart_scene = pName;
}

engine::fvector game_settings_loader::get_screen_size() const
{
	return mScreen_size;
}

void game_settings_loader::set_target_size(engine::fvector pSize)
{
	mScreen_size = pSize;
}

engine::ivector game_settings_loader::get_window_size() const
{
	return mWindow_size;
}

void game_settings_loader::set_window_size(engine::ivector pSize)
{
	mWindow_size = pSize;
}

float game_settings_loader::get_unit_pixels() const
{
	return pUnit_pixels;
}

void game_settings_loader::set_unit_pixels(float pUnit)
{
	pUnit_pixels = pUnit;
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

	auto ele_title = ele_root->FirstChildElement("title");
	if (!ele_title || !ele_title->GetText())
	{
		logger::warning("Please specify title of game.");
		mTitle = "[Unititled]";
	}
	else
	{
		mTitle = ele_title->GetText();
	}

	auto ele_scene = ele_root->FirstChildElement("scene");
	if (!ele_scene || !ele_scene->Attribute("name"))
	{
		logger::error("Please specify the scene to start with");
		return false;
	}
	mStart_scene = util::safe_string(ele_scene->Attribute("name"));

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
	mScreen_size = {
		ele_screen_size->FloatAttribute("x"),
		ele_screen_size->FloatAttribute("y")
	};
	
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
	mKey_bindings.clear();

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
			std::shared_ptr<scene_script_context> context(std::make_shared<scene_script_context>());
			//context->set_path(script_path.parent_path() + (script_path.stem().string() + ".as"));
			//pResource_manager.add_resource(engine::resource_type::script, , context);
		}
	}
	return true;
}

void scenes_directory::set_path(const std::string & pFilepath)
{
	mPath = pFilepath;
}

void scenes_directory::set_script_system(script_system & pScript)
{
	mScript = &pScript;
}
