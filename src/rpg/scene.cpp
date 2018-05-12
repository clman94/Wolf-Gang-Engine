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

void scene::cleanup(bool pFull)
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
	mLight_shader_manager.clear();

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

	cleanup();

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

	if (auto collision_boxes = mLoader.get_collisionboxes())
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
		}
		else
		{
			mTilemap_manipulator.set_texture(tilemap_texture);
			mTilemap_manipulator.load_xml(mLoader.get_tilemap());

			mTilemap_display.set_texture(tilemap_texture);
			mTilemap_display.update(mTilemap_manipulator);
		}
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

	cleanup(true);
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
	mLight_shader_manager.load_script_interface(pScript);

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
	mLight_shader_manager.set_renderer(pR);

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

#define STRINGIFY(A) #A

const char* const lightshader_frag = STRINGIFY(

	precision highp float;

	uniform sampler2D texture;

	uniform ivec2 target_size;

	uniform int brightness_levels;

	struct lightsource
	{
		vec2 xy;
		vec3 color; // This represents both color and brightness
		float radius;
		float atten_radius;
	};
	const int max_lights = 16;
	uniform lightsource lights[max_lights];

	vec3 invert(vec3 pColor)
	{
		return vec3(1, 1, 1) - pColor;
	}

	vec3 blend_burn(vec3 pColor, vec3 pBlend)
	{
		return invert(invert(pColor) / pBlend);
	}

	vec3 blend_linear_burn(vec3 pColor, vec3 pBlend)
	{
		return pColor + pBlend - vec3(1, 1, 1);
	}

	vec3 blend_overlay(vec3 pColor, vec3 pBlend)
	{
		return invert(invert(pColor) / invert(pBlend));
	}

	vec2 calc_pixelated_uv()
	{
		vec2 p = floor(gl_TexCoord[0].xy*vec2(target_size));
		p.y = vec2(target_size).y - p.y;
		return p;
	}

	vec3 calc_total_light_color()
	{
		vec2 pixelated_uv = calc_pixelated_uv();

		vec3 color = vec3(0, 0, 0);
		for (int i = 0; i < max_lights; i++)
		{
			if (lights[i].radius <= 0.0)
				continue;
			float d = distance(lights[i].xy, pixelated_uv);
			if (d <= lights[i].radius)
				color += lights[i].color;
			else if (lights[i].atten_radius > 0.0)
				color += max(1.0 - (d - lights[i].radius) / lights[i].atten_radius, 0.0)*lights[i].color;
		}
		return clamp(color, 0.0, 1.0);
	}

	vec3 calc_lights(vec3 pColor)
	{
		const vec3 darkBlend = vec3(0, 0, 0.3);
		const vec3 brightBlend = vec3(1, 1, 1);

		vec3 light_color = calc_total_light_color();

		float stepping_scalar = 1.0 / float(brightness_levels);
		light_color = floor(light_color / stepping_scalar) * stepping_scalar;

		float brightness = length(light_color);

		vec3 blend = mix(darkBlend, brightBlend, brightness);
		return mix(blend_linear_burn(pColor, blend), pColor*light_color, 0.8);
	}

	void main(void)
	{
		vec3 color = texture2D(texture, gl_TexCoord[0].xy).xyz;
		gl_FragColor = vec4(calc_lights(color), 1.0);
	}
);

const char* const lightshader_vert = STRINGIFY(
	void main()
	{
		// transform the vertex position
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

		// transform the texture coordinates
		gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

		// forward the vertex color
		gl_FrontColor = gl_Color;
	}
);

static inline std::string gen_lights_name(int pIdx)
{
	return std::string("lights[") + std::to_string(pIdx) + "]";
}

static inline void set_light_position(std::shared_ptr<engine::shader> pShader, int pIdx, const engine::fvector & pPos)
{
	if (pIdx < 0 || pIdx >= 16)
		return;
	pShader->set_uniform(gen_lights_name(pIdx) + ".xy", pPos);
}

static inline void set_light_color(std::shared_ptr<engine::shader> pShader, int pIdx, const engine::color & pColor)
{
	if (pIdx < 0 || pIdx >= 16)
		return;
	pShader->set_uniform(gen_lights_name(pIdx) + ".color", pColor);
}

static inline void set_light_radius(std::shared_ptr<engine::shader> pShader, int pIdx, float pPixels)
{
	if (pIdx < 0 || pIdx >= 16)
		return;
	pShader->set_uniform(gen_lights_name(pIdx) + ".radius", pPixels);
}

static inline void set_light_atten_radius(std::shared_ptr<engine::shader> pShader, int pIdx, float pPixels)
{
	if (pIdx < 0 || pIdx >= 16)
		return;
	pShader->set_uniform(gen_lights_name(pIdx) + ".atten_radius", pPixels);
}


light_shader_manager::light_shader_manager()
{
	mRenderer = nullptr;

}

void light_shader_manager::load_script_interface(script_system & pScript)
{
	mScript_system = &pScript;
	pScript.begin_namespace("light");
	pScript.add_function("initialize", &light_shader_manager::script_initialize, this);
	pScript.add_function("add", &light_shader_manager::script_add_light, this);
	pScript.add_function("remove", &light_shader_manager::script_remove_light, this);
	pScript.add_function("set_color", &light_shader_manager::script_set_color, this);
	pScript.add_function("get_color", &light_shader_manager::script_get_color, this);
	pScript.add_function("set_radius", &light_shader_manager::script_set_radius, this);
	pScript.add_function("get_radius", &light_shader_manager::script_get_radius, this);
	pScript.add_function("set_attenuation", &light_shader_manager::script_set_attenuation, this);
	pScript.add_function("get_attenuation", &light_shader_manager::script_get_attenuation, this);
	pScript.end_namespace();
}

void light_shader_manager::set_renderer(engine::renderer & pRenderer)
{
	mRenderer = &pRenderer;
	if (!mShader)
	{
		mShader = std::make_shared<engine::shader>();
		if (mShader->load(lightshader_vert, lightshader_frag))
			logger::info("Successfully compiled shader");
		else
		{
			logger::error("Shader failed to compile");
			return;
		}
	}
	mShader->set_uniform("brightness_levels", 5);
	for (std::size_t i = 0; i < mLight_entities.size(); i++)
	{
		mLight_entities[i].set_depth(rpg::defs::TILE_DEPTH_RANGE_MAX);
		mLight_entities[i].set_renderer(pRenderer);
		mLight_entities[i].set_light(static_cast<int>(i), mShader);
		//mLight_entities[i].reset_light();
	}
}

void light_shader_manager::clear()
{
	for (auto& i : mLight_entities)
		i.reset_light();
	mRenderer->set_shader(nullptr, rpg::defs::FX_DEPTH);
}

light_entity* light_shader_manager::cast_light_entity(entity_reference & pE)
{
	if (!pE.is_valid())
	{
		logger::print(*mScript_system, logger::level::error
			, "Entity object invalid.");
		return nullptr;
	}
	auto* l = dynamic_cast<light_entity*>(pE.get());
	if (!l)
	{
		logger::print(*mScript_system, logger::level::error
			, "This is not a light entity.");
		return nullptr;
	}
	return l;
}

entity_reference light_shader_manager::script_add_light()
{
	for (auto& i : mLight_entities)
	{
		if (!i.is_enabled())
		{
			i.set_enabled(true);
			i.set_visible(true);
			return i;
		}
	}
	return {};
}

void light_shader_manager::script_remove_light(entity_reference & pE)
{
	light_entity* l = nullptr;
	if (!(l = cast_light_entity(pE))) return;
	l->reset_light();
}

void light_shader_manager::script_initialize()
{
	mRenderer->set_shader(mShader, rpg::defs::FX_DEPTH);
}

void light_shader_manager::script_set_color(entity_reference& pE, const engine::color & pColor)
{
	light_entity* l = nullptr;
	if (!(l = cast_light_entity(pE))) return;
	l->set_color(pColor);
}

engine::color light_shader_manager::script_get_color(entity_reference & pE)
{
	light_entity* l = nullptr;
	if (!(l = cast_light_entity(pE))) return{};
	return l->get_color();
}

void light_shader_manager::script_set_radius(entity_reference& pE, float pPixels)
{
	light_entity* l = nullptr;
	if (!(l = cast_light_entity(pE))) return;
	l->set_radius(pPixels);
}

float light_shader_manager::script_get_radius(entity_reference & pE)
{
	light_entity* l = nullptr;
	if (!(l = cast_light_entity(pE))) return 0;
	return l->get_radius();
}

void light_shader_manager::script_set_attenuation(entity_reference& pE, float pPixels)
{
	light_entity* l = nullptr;
	if (!(l = cast_light_entity(pE))) return;
	l->set_atten_radius(pPixels);
}

float light_shader_manager::script_get_attenuation(entity_reference & pE)
{
	light_entity* l = nullptr;
	if (!(l = cast_light_entity(pE))) return 0;
	return l->get_atten_radius();
}


light_entity::light_entity()
{
	mIs_enabled = false;
	mRadius = 0;
	mAtten_radius = 0;
	mLight_idx = -1;
}

light_entity::~light_entity()
{
	reset_light();
}

void light_entity::reset_light()
{
	set_position(engine::fvector(0, 0));
	set_color(engine::color(0, 0, 0, 0));
	set_radius(0);
	set_atten_radius(0);
	set_position({ 0, 0 });
	detach_parent();
	detach_children();
	set_visible(false);
	mIs_enabled = false;
}

int light_entity::draw(engine::renderer & pR)
{
	if (mShader)
	{
		engine::fvector pos = get_exact_position();
		set_light_radius(mShader, mLight_idx, mRadius);
		set_light_atten_radius(mShader, mLight_idx, mAtten_radius);
		set_light_color(mShader, mLight_idx, mColor);
		set_light_position(mShader, mLight_idx, pos);
	}
	return 0;
}

int light_entity::get_light_index() const
{
	return mLight_idx;
}

void light_entity::set_light(int pIdx, std::shared_ptr<engine::shader> pShader)
{
	mLight_idx = pIdx;
	mShader = pShader;
}

void light_entity::set_radius(float pPixels)
{
	mRadius = pPixels;
	if (mShader)
		set_light_radius(mShader, mLight_idx, mRadius);
}

float light_entity::get_radius() const
{
	return mRadius;
}

void light_entity::set_atten_radius(float pPixels)
{
	mAtten_radius = pPixels;
	if (mShader)
		set_light_atten_radius(mShader, mLight_idx, mAtten_radius);
}

float light_entity::get_atten_radius() const
{
	return mAtten_radius;
}

void light_entity::set_color(const engine::color & pColor)
{
	mColor = pColor;
	if (mShader)
		set_light_color(mShader, mLight_idx, mColor);
}

const engine::color& light_entity::get_color() const
{
	return mColor;
}

void light_entity::set_enabled(bool pEnabled)
{
	mIs_enabled = pEnabled;
}

bool light_entity::is_enabled() const
{
	return mIs_enabled;
}
