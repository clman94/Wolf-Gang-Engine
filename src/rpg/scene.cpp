#include <rpg/scene.hpp>
#include <rpg/rpg_config.hpp>
#include <engine/logger.hpp>

using namespace rpg;

// #########
// scene
// #########

scene::scene()
{
	mTilemap_display.set_depth(defs::TILES_DEPTH);

	mWorld_node.set_parent(mScene_node);

	mWorld_node.add_child(mTilemap_display);
	mFocus_player = true;

	mPathfinding_system.set_collision_system(mCollision_system);

	mEntity_manager.set_world_node(mWorld_node);
	mEntity_manager.set_scene_node(mScene_node);

	mPack = nullptr;
	mResource_manager = nullptr;

	mIs_ready = false;

	mSound_FX.attach_mixer(mMixer);
	mBackground_music.set_mixer(mMixer);
}

scene::~scene()
{
	logger::info("Destroying scene");
}

panning_node& scene::get_world_node()
{
	return mWorld_node;
}

engine::node & scene::get_scene_node()
{
	return mScene_node;
}

collision_system& scene::get_collision_system()
{
	return mCollision_system;
}

void scene::clean(bool pFull)
{
	mScript->abort_all();

	mEnd_functions.clear();

	mTilemap_display.clear();
	mTilemap_manipulator.clear();
	mCollision_system.clear();
	mEntity_manager.clear();
	mColored_overlay.reset();
	mSound_FX.stop_all();
	mBackground_music.pause_music();

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
	assert(!mData_path.empty());

	clean();

	mCurrent_scene_name = pName;

	logger::info("Loading scene '" + pName + "'");

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
		if (!mLoader.load((mData_path / defs::DEFAULT_SCENES_PATH).string(), pName))
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
			context.build_script(mLoader.get_script_path(), mData_path);
	}
	else
		logger::info("Script is already compiled");

	// Setup context if still valid
	if (context.is_valid())
	{
		context.clear_globals();
		mCollision_system.setup_script_defined_triggers(context);
		context.call_all_with_tag("start");
		mEnd_functions = context.get_all_with_tag("door");
	}

	if (mLoader.get_tilemap_texture().empty())
	{
		logger::info("No tilemap texture");
	}
	else
	{
		logger::info("Loading Tilemap...");

		auto tilemap_texture = mResource_manager->get_resource<engine::texture>("texture", mLoader.get_tilemap_texture());
		if (!tilemap_texture)
		{
			logger::error("Invalid tilemap texture");
			return false;
		}
		mTilemap_manipulator.set_texture(tilemap_texture);
		mTilemap_manipulator.load_xml(mLoader.get_tilemap());

		mTilemap_display.set_texture(tilemap_texture);
		mTilemap_display.update(mTilemap_manipulator);
	}

	// Pre-execute so the scene script can setup things before the first render.
	mScript->tick();

	logger::info("Cleaning up resources...");

	// Ensure resources are ready and unused stuff is put away
	mResource_manager->ensure_load();
	mResource_manager->unload_unused();

	mIs_ready = true;
	return true;
}

bool scene::reload_scene()
{
	if (mCurrent_scene_name.empty())
	{
		logger::error("No scene to reload");
		return false;
	}

	clean(true);
	mResource_manager->reload_all();
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

	pScript.add_function("_set_focus", &scene::script_set_focus, this);
	pScript.add_function("_get_focus", &scene::script_get_focus, this);

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

	mTerminal_cmd_group->add_command("resources",
		[&](const engine::terminal_arglist& pArgs)->bool
	{
		logger::info(mResource_manager->get_resource_log());
		return true;
	}, "- Display resource info");

	pTerminal.add_group(mTerminal_cmd_group);

}
#endif

void scene::set_resource_manager(engine::resource_manager& pResource_manager)
{
	mResource_manager = &pResource_manager;
	mEntity_manager.set_resource_manager(pResource_manager);
	mBackground_music.set_resource_manager(pResource_manager);
}

bool scene::load_settings(const game_settings_loader& pSettings)
{
	logger::info("Loading scene system...");

	mScene_node.set_unit(pSettings.get_unit_pixels());
	mWorld_node.set_viewport(pSettings.get_screen_size());

	logger::info("Scene loaded");

	return true;
}

void scene::set_resource_pack(engine::resource_pack* pPack)
{
	mPack = pPack;
}

scene_visualizer & scene::get_visualizer()
{
	return mVisualizer;
}

bool scene::is_ready() const
{
	return mIs_ready;
}

engine::mixer & scene::get_mixer()
{
	return mMixer;
}

engine::fvector scene::get_camera_focus() const
{
	return mWorld_node.get_focus();
}

void scene::set_data_source(const engine::fs::path & pPath)
{
	mData_path = pPath;
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
	auto sound = mResource_manager->get_resource<engine::sound_file>("audio", pName);
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
	mTilemap_manipulator.get_layer(pLayer).set_tile(pPosition, pAtlas, pRotation);
	mTilemap_display.update(mTilemap_manipulator);
}

void scene::script_remove_tile(engine::fvector pPosition, int pLayer)
{
	mTilemap_manipulator.get_layer(pLayer).remove_tile(pPosition);
	mTilemap_display.update(mTilemap_manipulator);
}

void scene::refresh_renderer(engine::renderer& pR)
{
	mWorld_node.set_viewport(pR.get_target_size());
	pR.add_object(mTilemap_display);
	mColored_overlay.set_renderer(pR);
	mEntity_manager.set_renderer(pR);

#ifndef LOCKED_RELEASE_MODE
	mVisualizer.set_depth(-100);
	mVisualizer.set_scene(*this);
	mVisualizer.set_renderer(pR);
#endif
}

scene_visualizer::scene_visualizer()
{
	mEntity_center_visualize.set_color(engine::color(1, 1, 0, 0.78f));
	mEntity_center_visualize.set_size(engine::fvector(2, 2));

	mEntity_visualize.set_outline_color(engine::color(0, 1, 0, 0.78f));
	mEntity_visualize.set_color(engine::color_preset::black);
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
	mBox_visualize.set_color(engine::color(0, 1, 1, 0.27f));
	for (const auto& i : mScene->get_collision_system().get_container())
	{
		mBox_visualize.set_position(i->get_region().get_offset() + mScene->get_world_node().get_absolute_position());
		mBox_visualize.set_size(i->get_region().get_size()*mBox_visualize.get_unit());
		mBox_visualize.draw(pR);
	}
}
