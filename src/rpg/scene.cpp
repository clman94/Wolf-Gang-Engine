#include <rpg/scene.hpp>
#include <rpg/rpg_config.hpp>
#include <engine/log.hpp>

using namespace rpg;

// #########
// scene
// #########

scene::scene()
{
	mTilemap_display.set_depth(defs::TILES_DEPTH);

	mWorld_node.add_child(mTilemap_display);
	mWorld_node.add_child(mPlayer);
	mFocus_player = true;

	mPathfinding_system.set_collision_system(mCollision_system);

	mEntity_manager.set_root_node(mWorld_node);

	mPack = nullptr;
	mResource_manager = nullptr;

	mIs_ready = false;
}

scene::~scene()
{
	logger::info("Destroying scene");
}

panning_node& scene::get_world_node()
{
	return mWorld_node;
}

collision_system&
scene::get_collision_system()
{
	return mCollision_system;
}

void
scene::clean(bool pFull)
{
	mScript->abort_all();

	mEnd_functions.clear();

	mTilemap_display.clean();
	mTilemap_manipulator.clean();
	mCollision_system.clean();
	mEntity_manager.clean();
	mColored_overlay.clean();
	mSound_FX.stop_all();
	mBackground_music.pause_music();

	focus_player(true);

	mPlayer.clean();

	if (pFull) // Cleanup everything
	{
		mBackground_music.clean();

		// Clear all contexts for recompiling
		for (auto &i : pScript_contexts)
			i.second.clean();
		pScript_contexts.clear();
	}

	mIs_ready = false;
}

bool scene::load_scene(std::string pName)
{
	assert(mResource_manager != nullptr);
	assert(mScript != nullptr);

	clean();

	mCurrent_scene_name = pName;

	logger::info("Loading scene '" + pName + "'");
	logger::sub_routine _srtn_loading_scene;

	if (mPack)
	{
		if (!mLoader.load(defs::DEFAULT_SCENES_PATH.string(), pName, *mPack))
		{
			logger::error("Unable to open scene '" + pName + "'");
			return false;
		}
	}
	else
	{
		if (!mLoader.load((defs::DEFAULT_DATA_PATH / defs::DEFAULT_SCENES_PATH).string(), pName))
		{
			logger::error("Unable to open scene '" + pName + "'");
			return false;
		}
	}

	auto collision_boxes = mLoader.get_collisionboxes();
	if (collision_boxes)
		mCollision_system.load_collision_boxes(collision_boxes);

	mWorld_node.set_boundary_enable(mLoader.has_boundary());
	mWorld_node.set_boundary(mLoader.get_boundary());

	auto &context = pScript_contexts[mLoader.get_name()];

	// Compile script if not already
	if (!context.is_valid())
	{
		context.set_script_system(*mScript);
		if (mPack)
			context.build_script(mLoader.get_script_path(), *mPack);
		else
			context.build_script(mLoader.get_script_path());
	}
	else
		logger::info("Script is already compiled");

	// Setup context if still valid
	if (context.is_valid())
	{
		context.clean_globals();
		mCollision_system.setup_script_defined_triggers(context);
		context.start_all_with_tag("start");
		mEnd_functions = context.get_all_with_tag("door");
	}



	if (mLoader.get_tilemap_texture().empty())
	{
		logger::info("No tilemap texture");
	}
	else
	{
		logger::info("Loading Tilemap...");
		logger::sub_routine _srtn_loading_tilemap;

		auto tilemap_texture = mResource_manager->get_resource<engine::texture>(engine::resource_type::texture, mLoader.get_tilemap_texture());
		if (!tilemap_texture)
		{
			logger::error("Invalid tilemap texture");
			return false;
		}
		mTilemap_display.set_texture(tilemap_texture);

		mTilemap_manipulator.load_tilemap_xml(mLoader.get_tilemap());
		mTilemap_manipulator.update_display(mTilemap_display);
	}

	mPlayer.set_visible(true);

	// Pre-execute so the scene script can setup things before the render.
	mScript->tick();

	update_focus();

	logger::info("Cleaning up resources...");
	logger::start_sub_routine();

	// Ensure resources are ready and unused stuff is put away
	mResource_manager->ensure_load();
	mResource_manager->unload_unused();

	logger::end_sub_routine();

	mIs_ready = true;
	return true;
}

bool scene::load_scene(std::string pName, std::string pDoor)
{
	if (!load_scene(pName))
		return false;

	auto door = mCollision_system.get_door_entry(pDoor);
	if (!door)
	{
		logger::warning("Unable to find door '" + pDoor + "'");
		return false;
	}
	mPlayer.set_direction(character_entity::vector_direction(door->get_offset()));
	mPlayer.set_position(door->calculate_player_position());
	return true;
}

#ifndef LOCKED_RELEASE_MODE
bool scene::create_scene(const std::string & pName)
{
	const auto xml_path = defs::DEFAULT_DATA_PATH / defs::DEFAULT_SCENES_PATH / (pName + ".xml");
	const auto script_path = defs::DEFAULT_DATA_PATH / defs::DEFAULT_SCENES_PATH / (pName + ".as");

	if (engine::fs::exists(xml_path) || engine::fs::exists(script_path))
	{
		logger::error("Scene '" + pName + "' already exists");
		return false;
	}

	// Ensure the existance of the directories
	engine::fs::create_directories(xml_path.parent_path());

	// Create xml file
	std::ofstream xml(xml_path.string().c_str());
	xml << defs::MINIMAL_XML_SCENE;
	xml.close();

	// Create script file
	std::ofstream script(script_path.string().c_str());
	script << defs::MINIMAL_SCRIPT_SCENE;
	script.close();

	return true;
}
#endif

bool scene::reload_scene()
{
	if (mCurrent_scene_name.empty())
	{
		logger::error("No scene to reload");
		return 1;
	}

	clean(true);

	return load_scene(mCurrent_scene_name);
}

const std::string& scene::get_path()
{
	return mLoader.get_name();
}

const std::string& scene::get_name()
{
	return mLoader.get_name();
}

void scene::load_script_interface(script_system& pScript)
{
	mEntity_manager.load_script_interface(pScript);
	mBackground_music.load_script_interface(pScript);
	mColored_overlay.load_script_interface(pScript);
	mPathfinding_system.load_script_interface(pScript);
	mCollision_system.load_script_interface(pScript);

	pScript.add_function("set_tile", &scene::script_set_tile, this);
	pScript.add_function("remove_tile", &scene::script_remove_tile, this);

	pScript.add_function("_spawn_sound", &scene::script_spawn_sound, this);
	pScript.add_function("_stop_all", &engine::sound_spawner::stop_all, &mSound_FX);

	pScript.add_function("get_player", &scene::script_get_player, this);
	pScript.add_function("_set_player_locked", &player_character::set_locked, &mPlayer);
	pScript.add_function("_get_player_locked", &player_character::is_locked, &mPlayer);

	pScript.add_function("_set_focus", &scene::script_set_focus, this);
	pScript.add_function("_get_focus", &scene::script_get_focus, this);
	pScript.add_function("_focus_player", &scene::focus_player, this);

	pScript.add_function("get_boundary_position", &scene::script_get_boundary_position, this);
	pScript.add_function("get_boundary_size", &scene::script_get_boundary_size, this);
	pScript.add_function("get_boundary_position", &scene::script_set_boundary_position, this);
	pScript.add_function("get_boundary_size", &scene::script_set_boundary_size, this);
	pScript.add_function("set_boundary_enable", &panning_node::set_boundary_enable, this);

	pScript.add_function("get_display_size", &scene::script_get_display_size, this);

	pScript.add_function("get_scene_name", &scene::get_name, this);

	mScript = &pScript;
}

#ifndef LOCKED_RELEASE_MODE
void scene::load_terminal_interface(engine::terminal_system & pTerminal)
{
	mVisualizer.load_terminal_interface(pTerminal);

	mTerminal_cmd_group = std::make_shared<engine::terminal_command_group>();

	mTerminal_cmd_group->set_root_command("scene");

	mTerminal_cmd_group->add_command("reload",
		[&](const std::vector<engine::terminal_argument>& pArgs)->bool
	{
		return reload_scene();
	}, "- Reload the scene");

	mTerminal_cmd_group->add_command("load",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.size() <= 0)
		{
			logger::error("Not enough arguments");
			logger::info("scene load <scene_name>");
			return false;
		}

		return load_scene(pArgs[0]);
	}, "<Scene Name> - Load a scene by name");

	mTerminal_cmd_group->add_command("create",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.size() <= 0)
		{
			logger::error("Not enough arguments");
			logger::info("scene create <scene_name>");
			return false;
		}

		if (!create_scene(pArgs[0]))
			return false;

		return load_scene(pArgs[0]);
	}, "<Scene Name> - Create a new scene");

	pTerminal.add_group(mTerminal_cmd_group);

}
#endif

void scene::set_resource_manager(engine::resource_manager& pResource_manager)
{
	mResource_manager = &pResource_manager;
	mEntity_manager.set_resource_manager(pResource_manager);
}

bool scene::load_settings(const game_settings_loader& pSettings)
{
	logger::info("Loading scene system...");

	mWorld_node.set_unit(pSettings.get_unit_pixels());

	mWorld_node.set_viewport(pSettings.get_screen_size());

	auto texture = mResource_manager->get_resource<engine::texture>(engine::resource_type::texture, pSettings.get_player_texture());
	if (!texture)
	{
		logger::error("Could not load texture '" + pSettings.get_player_texture() + "' for player character");
		return false;
	}
	mPlayer.mSprite.set_texture(texture);
	mPlayer.set_cycle(character_entity::cycle::def);

	mBackground_music.set_root_directory(pSettings.get_music_path());

	logger::info("Scene loaded");

	return true;
}

player_character& scene::get_player()
{
	return mPlayer;
}

void scene::tick(engine::controls &pControls)
{
	assert(get_renderer() != nullptr);
	mPlayer.movement(pControls, mCollision_system, get_renderer()->get_delta());
	update_focus();
	update_collision_interaction(pControls);
}

void scene::focus_player(bool pFocus)
{
	mFocus_player = pFocus;
}

void scene::set_resource_pack(engine::pack_stream_factory* pPack)
{
	mPack = pPack;
	mBackground_music.set_resource_pack(pPack);
}

scene_visualizer & scene::get_visualizer()
{
	return mVisualizer;
}

bool scene::is_ready() const
{
	return mIs_ready;
}

void scene::script_set_focus(engine::fvector pPosition)
{
	mFocus_player = false;
	mWorld_node.set_focus(pPosition);
}

engine::fvector scene::script_get_focus()
{
	return mWorld_node.get_focus();
}

entity_reference scene::script_get_player()
{
	return mPlayer;
}

engine::fvector scene::script_get_boundary_position()
{
	return mWorld_node.get_boundary().get_offset();
}

engine::fvector scene::script_get_boundary_size()
{
	return mWorld_node.get_boundary().get_size();
}

void scene::script_set_boundary_size(engine::fvector pSize)
{
	mWorld_node.set_boundary(mWorld_node.get_boundary().set_size(pSize));
}

void scene::script_spawn_sound(const std::string & pName, float pVolume, float pPitch)
{
	auto sound = mResource_manager->get_resource<engine::sound_buffer>(engine::resource_type::sound, pName);
	if (!sound)
	{
		logger::error("Could not spawn sound '" + pName + "'");
		return;
	}

	mSound_FX.spawn(sound, pVolume, pPitch);
}

engine::fvector scene::script_get_display_size()
{
	assert(get_renderer() != nullptr);
	return get_renderer()->get_target_size();
}

void scene::script_set_boundary_position(engine::fvector pPosition)
{
	mWorld_node.set_boundary(mWorld_node.get_boundary().set_offset(pPosition));
}

void scene::script_set_tile(const std::string& pAtlas, engine::fvector pPosition
	, int pLayer, int pRotation)
{
	mTilemap_manipulator.set_tile(pPosition, pLayer, pAtlas, pRotation);
	mTilemap_manipulator.update_display(mTilemap_display);
}

void scene::script_remove_tile(engine::fvector pPosition, int pLayer)
{
	mTilemap_manipulator.remove_tile(pPosition, pLayer);
	mTilemap_manipulator.update_display(mTilemap_display);
}

void scene::refresh_renderer(engine::renderer& pR)
{
	mWorld_node.set_viewport(pR.get_target_size());
	pR.add_object(mTilemap_display);
	pR.add_object(mPlayer);
	mColored_overlay.set_renderer(pR);
	mEntity_manager.set_renderer(pR);

#ifndef LOCKED_RELEASE_MODE
	mVisualizer.set_depth(-100);
	mVisualizer.set_scene(*this);
	mVisualizer.set_renderer(pR);
#endif
}

void scene::update_focus()
{
	if (mFocus_player)
		mWorld_node.set_focus(mPlayer.get_position(mWorld_node));
}

void scene::update_collision_interaction(engine::controls & pControls)
{
	collision_box_container& container = mCollision_system.get_container();

	const auto collision_box = mPlayer.get_collision_box();

	// Check collision with triggers
	{
		auto triggers = container.collision(collision_box::type::trigger, collision_box);
		for (auto& i : triggers)
			std::dynamic_pointer_cast<trigger>(i)->call_function();
	}

	// Check collision with doors
	{
		const auto hit = container.first_collision(collision_box::type::door, collision_box);
		if (hit)
		{
			const auto hit_door = std::dynamic_pointer_cast<door>(hit);
			if (mEnd_functions.empty()) // No end functions to call
			{
				load_scene(hit_door->get_scene(), hit_door->get_destination());
			}
			else if (!mEnd_functions[0]->is_running())
			{
				mScript->abort_all();
				for (auto& i : mEnd_functions)
				{
					if (i->call())
					{
						i->set_arg(0, (void*)&hit_door->get_scene());
						i->set_arg(1, (void*)&hit_door->get_destination());
					}
				}
			}
		}
	}

	// Check collision with buttons on when "activate" is triggered
	if (pControls.is_triggered("activate"))
	{
		auto buttons = container.collision(collision_box::type::button, mPlayer.get_activation_point());
		for (auto& i : buttons)
			std::dynamic_pointer_cast<button>(i)->call_function();
	}
}

scene_visualizer::scene_visualizer()
{
	mEntity_center_visualize.set_color(engine::color(255, 255, 0, 200));
	mEntity_center_visualize.set_size(engine::fvector(2, 2));

	mEntity_visualize.set_outline_color(engine::color(0, 255, 0, 200));
	mEntity_visualize.set_color(engine::color(0, 0, 0, 0));
	mEntity_visualize.set_outline_thinkness(1);

	mVisualize_entities = false;
	mVisualize_collision = false;
}

void scene_visualizer::set_scene(scene & pScene)
{
	mScene = &pScene;
}

void scene_visualizer::visualize_entities(bool pVisualize)
{
	mVisualize_entities = pVisualize;
}

void scene_visualizer::visualize_collision(bool pVisualize)
{
	mVisualize_collision = pVisualize;
}

void scene_visualizer::load_terminal_interface(engine::terminal_system & pTerminal_system)
{
	mGroup = std::make_shared<engine::terminal_command_group>();
	mGroup->set_root_command("visualize");

	mGroup->add_command("collision", [&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.size() >= 1 && pArgs[0].get_raw() == "off")
			mVisualize_collision = false;
		else
			mVisualize_collision = true;
		return true;
	}, "[off] - Visualize collision");

	mGroup->add_command("entities", [&](const engine::terminal_arglist& pArgs)->bool
	{
		if (pArgs.size() >= 1 && pArgs[0].get_raw() == "off")
			mVisualize_entities = false;
		else
			mVisualize_entities = true;
		return true;
	}, "[off] - Visualize entities");

	pTerminal_system.add_group(mGroup);
}

int scene_visualizer::draw(engine::renderer & pR)
{
	if (mVisualize_collision)
		visualize_collision(pR);
	if (mVisualize_entities)
		visualize_entities(pR);
	return 0;
}

void scene_visualizer::visualize_entities(engine::renderer & pR)
{
	for (const auto& i : mScene->mEntity_manager.mEntities)
	{
		if (i->get_type() == rpg::entity::type::sprite)
		{
			const auto e = dynamic_cast<sprite_entity*>(i.get());
			const engine::frect rect(e->mSprite.get_render_rect());
			mEntity_visualize.set_position(rect.get_offset());
			mEntity_visualize.set_size(rect.get_size());
		}
		else if (i->get_type() == entity::type::text)
		{
			const auto e = dynamic_cast<text_entity*>(i.get());
			const engine::frect rect(e->mText.get_render_rect());
			mEntity_visualize.set_position(rect.get_offset());
			mEntity_visualize.set_size(rect.get_size());
		}

		mEntity_visualize.draw(pR);

		mEntity_center_visualize.set_absolute_position(i->get_exact_position());
		mEntity_center_visualize.draw(pR);
	}
}

void scene_visualizer::visualize_collision(engine::renderer & pR)
{
	mBox_visualize.set_unit(mScene->get_world_node().get_unit());
	mBox_visualize.set_color(engine::color(0, 255, 255, 70));
	for (const auto& i : mScene->get_collision_system().get_container())
	{
		mBox_visualize.set_position(i->get_region().get_offset() + mScene->get_world_node().get_absolute_position());
		mBox_visualize.set_size(i->get_region().get_size()*mBox_visualize.get_unit());
		mBox_visualize.draw(pR);
	}
}
