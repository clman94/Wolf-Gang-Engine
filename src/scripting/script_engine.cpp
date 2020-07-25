#include <wge/scripting/script_engine.hpp>
#include <wge/physics/physics_world.hpp>
#include <Box2D/Dynamics/b2World.h>
#include <wge/core/object_resource.hpp>
#include <wge/math/transform.hpp>
#include <wge/math/rect.hpp>

#include <regex>

namespace wge::scripting
{

static std::string_view parse_lua_string_source(std::string_view pStr)
{
	const std::regex grammer("\\[string\\s\"(.*)\"\\]");
	std::match_results<std::string_view::const_iterator> results;
	if (std::regex_match(pStr.begin(), pStr.end(), results, grammer))
		return std::string_view(&*results[1].first, results[1].second - results[1].first);
	return pStr;
}

static std::pair<std::string_view, std::string_view> parse_source_id(std::string_view pStr)
{
	auto begin = std::find(pStr.begin(), pStr.end(), '[') + 1; // +1 to skip '['
	if (begin == pStr.end())
		return { pStr, {} };
	auto end = std::find(begin, pStr.end(), ']');
	if (end == pStr.end())
		return { pStr, {} };
	if (std::distance(begin, end) != 36)
		return { pStr, {} };
	return { std::string_view(&*pStr.begin(), (begin - 1) - pStr.begin()),
		std::string_view(&*begin, end - begin) };
}

static error_info parse_lua_error(std::string_view pStr)
{
	error_info result;
	const std::regex message_rule("(.*):(\\d+):\\s*([\\s\\S]*)");
	std::match_results<std::string_view::const_iterator> results;
	if (std::regex_match(pStr.begin(), pStr.end(), results, message_rule))
	{
		// Extract the string from "[string "{}"]"
		result.source = parse_lua_string_source(results[1].str());
		// Convert the line number.
		result.line = std::atoi(results[2].str().c_str());
		// Get the message.
		result.message = results[3].str();
	}
	else
		result.message = pStr;
	return result;
}

static stack_entry parse_lua_stack_entry(std::string_view pStr)
{
	const std::regex line_parse("\\s*([\\s\\S]*?):(\\d+):\\sin\\s([\\s\\S]*)");
	std::match_results<std::string_view::const_iterator> results;
	if (std::regex_search(pStr.begin(), pStr.end(), results, line_parse))
	{
		stack_entry entry;
		entry.source = parse_lua_string_source(results[1].str());
		entry.line = std::stoi(results[2].str().c_str());
		entry.main_chunk = entry.source == "main chunk";
		entry.what = results[3].str();
		return entry;
	}
	return {};
}

static runtime_error_info parse_lua_runtime_error(std::string_view pStr)
{
	runtime_error_info result;
	const std::regex stacktrace_parse("\\nstack\\straceback:\\n");
	std::match_results<std::string_view::const_iterator> results;
	if (std::regex_search(pStr.begin(), pStr.end(), results, stacktrace_parse))
	{
		// Everything before "stack traceback:" is a normal lua error.
		static_cast<error_info&>(result) = parse_lua_error(results.prefix().str());

		// Split lines for each stack entry.
		auto begin = results[0].second;
		auto end = begin;
		do {
			begin = end;
			end = std::find(begin + 1, pStr.end(), '\n');
			result.stack.push_back(parse_lua_stack_entry(std::string_view{ &*begin, static_cast<std::size_t>(end - begin) }));
		} while (end != pStr.end());
	}
	else
		log::info("nomatch");
	return result;
}

sol::environment script_engine::create_object_environment(core::object pObj, sol::environment pExisting)
{
	sol::environment env = pExisting.valid() ? pExisting : sol::environment{ state, sol::create, state.globals() };

	// Register the object to be accessible through the obj table.
	if (!pObj.get_name().empty())
		state["obj"][pObj.get_name()] = env;

	env["alarm"] = state.create_table_with(
		1, 0,
		2, 0,
		3, 0,
		4, 0,
		5, 0,
		6, 0,
		7, 0,
		8, 0
	);

	env["_created"] = false;
	env["_unique_created"] = false;

	env["is_valid"] = [pObj]() -> bool
	{
		return pObj.is_valid();
	};

	env["this"] = env;

	auto get_position = [pObj]() -> math::vec2
	{
		return pObj.get_component<math::transform>()->position;
	};
	auto set_position = [pObj](const math::vec2& pPos)
	{
		pObj.get_component<math::transform>()->position = pPos;
	};

	env["get_scale"] = [pObj]() -> math::vec2
	{
		return pObj.get_component<math::transform>()->scale;
	};;
	env["set_scale"] = [pObj](const math::vec2& pScale)
	{
		pObj.get_component<math::transform>()->scale = pScale;
	};

	env["get_rotation"] = [pObj]() -> float
	{
		return math::degrees{ pObj.get_component<math::transform>()->rotation }.value();
	};;
	env["set_rotation"] = [pObj](float pDeg)
	{
		pObj.get_component<math::transform>()->rotation = math::degrees{ pDeg };
	};

	//env["position"] = sol::property(get_position, set_position);
	env["get_position"] = get_position;
	env["set_position"] = set_position;
	env["move"] = [get_position, set_position](const math::vec2& pDirection)
	{
		set_position(get_position() + pDirection);
	};

	env["destroy"] = [pObj]()
	{
		core::object{ pObj }.destroy(core::queue_destruction);
	};

	env["animation_play"] = [pObj]()
	{
		if (auto comp = pObj.get_component<graphics::sprite_component>())
			comp->get_controller().play();
	};

	env["animation_stop"] = [pObj]()
	{
		if (auto comp = pObj.get_component<graphics::sprite_component>())
			comp->get_controller().stop();
	};

	env["set_sprite"] = [pObj](const std::string& pName)
	{

	};

	env["this_layer"] = std::ref(pObj.get_layer());

	return env;
}

void script_engine::update_delta(float pSeconds)
{
	state["delta"] = pSeconds;
}

bool script_engine::compile_script(const script::handle& pScript, const std::string& pName)
{
	assert(pScript);
	auto& source = *pScript;
	if (!source.function.valid())
	{
		sol::load_result lr = state.load(source.source, fmt::format("{} [{}]", pName, pScript.get_id().to_string()));
		if (lr.valid())
		{
			source.function = lr;
		}
		else
		{
			sol::error err = lr;
			mCompile_errors[pScript.get_id()] = parse_lua_error(err.what());
			log::error("Parse error: {}", err.what());
			return false;
		}
	}
	return true;
}

void script_engine::cleanup()
{
	clear_errors();
	state["obj"] = state.create_table();
	state["global"] = sol::environment(state, sol::create, state.globals());
}

void script_engine::register_core_api()
{
	state.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string, sol::lib::table);
	state["obj"] = state.create_table();
	state["global"] = sol::environment(state, sol::create, state.globals());
	state["table"]["haskey"] = [](sol::table pTable, sol::object pKey) -> bool
	{
		for (auto& i : pTable)
			if (i.second != sol::nil && i.first == pKey)
				return true;
		return false;
	};

	state["delta"] = 1.f;
	state["dprint"] = sol::overload(
		[](const std::string& pVal) { log::debug("{}", pVal); },
		[](double pVal) { log::debug("{}", pVal); }
	);
}

void script_engine::register_asset_api(core::asset_manager& pAsset_manager)
{
}

void script_engine::register_layer_api(core::asset_manager& pAsset_manager)
{
	state["create_instance"] = [this, &pAsset_manager](core::layer& pLayer, std::string_view pStr) -> sol::table
	{
		if (auto asset = pAsset_manager.get_asset(filesystem::path(pStr)))
		{
			auto res = asset->get_resource<core::object_resource>();
			if (!res)
				log::error("Asset is not an instantiable object");
			auto obj = pLayer.add_object();
			res->generate_object(obj, pAsset_manager);
			if (auto comp = obj.get_component<event_state_component>())
			{
				comp->environment = create_object_environment(obj);
				return comp->environment;
			}
		}
		return{};
	};
}

void script_engine::register_graphics_api(graphics::camera& pDefault_camera)
{
	sol::table t = state.create_named_table("graphics");
	t.new_usertype<graphics::camera>("camera", 
		"focus", sol::property(&graphics::camera::get_focus, &graphics::camera::set_focus),
		"size", sol::property(&graphics::camera::get_size, &graphics::camera::set_size)
		);
	t["main_camera"] = std::ref(pDefault_camera);
}

void script_engine::register_math_api()
{
	sol::table t = state.create_named_table("math");
	t["pi"] = math::pi;
	t["exp"] = math::exp<float>;
	t["exp2"] = math::exp2<float>;
	t["sin"] = [](float pRad) { return math::sin(pRad); };
	t["cos"] = [](float pRad) { return math::cos(pRad); };
	t["abs"] = &math::abs<float>;
	t["floor"] = &math::floor<float>;
	t["round"] = &math::round<float>;
	t["max"] = &math::max<float>;
	t["min"] = &math::min<float>;
	t["clamp"] = sol::overload(&math::clamp<float>, &math::clamp_components<float>);
	t["sqrt"] = &math::sqrt<float>;
	t["mod"] = &math::mod<float>;
	t["pmod"] = &math::positive_modulus<float>;
	t["pow"] = &math::pow<float>;
	t["log"] = &math::log<float>;
	t["log2"] = &math::log2<float>;
	t["log10"] = &math::log10<float>;
	t["is_nan"] = &math::is_nan<float>;
	t["sign"] = &math::sign<float>;
	t.new_usertype<math::vec2>("vec2",
		sol::call_constructor, sol::constructors<math::vec2(),
		math::vec2(const math::vec2&),
		math::vec2(float, float)>(),
		"x", &math::vec2::x,
		"y", &math::vec2::y,
		"normalize", &math::vec2::normalize,
		"normal_to", &math::vec2::normal_to,
		"abs", &math::vec2::abs,
		"clone", [](const math::vec2& pVec) -> math::vec2 { return pVec; },
		"dot", &math::vec2::dot,
		"distance", &math::vec2::distance,
		"rotate", [](math::vec2& pVec, float pDeg) -> math::vec2 { return pVec.rotate(math::degrees(pDeg)); },
		"rotate_around", [](math::vec2& pVec, float pDeg, const math::vec2& pOther) -> math::vec2 { return pVec.rotate_around(math::degrees(pDeg), pOther); },
		"angle", [](const math::vec2& pVec) -> float { return math::degrees(pVec.angle()).value(); },
		"angle_to", [](const math::vec2& pVec, const math::vec2& pOther) -> float { return math::degrees(pVec.angle_to(pOther)).value(); },
		"mirror_x", &math::vec2::mirror_x,
		"mirror_y", &math::vec2::mirror_y,
		"mirror_xy", &math::vec2::mirror_xy,
		"set", &math::vec2::set,
		"swap_xy", &math::vec2::swap_xy,
		"is_nan", &math::vec2::is_nan,
		"is_zero", &math::vec2::is_zero,
		"magnitude", &math::vec2::magnitude,
		"project", &math::vec2::project,
		"reflect", &math::vec2::reflect,
		sol::meta_function::addition, sol::overload(
			static_cast<math::vec2(*)(const math::vec2&, const math::vec2&)>(math::operator+),
			static_cast<math::vec2(*)(const math::vec2&, float)>(&math::operator+)
		),
		sol::meta_function::subtraction, sol::overload(
			static_cast<math::vec2(*)(const math::vec2&, const math::vec2&)>(math::operator-),
			static_cast<math::vec2(*)(const math::vec2&, float)>(&math::operator-)
		),
		sol::meta_function::multiplication, sol::overload(
			static_cast<math::vec2(*)(const math::vec2&, const math::vec2&)>(math::operator*),
			static_cast<math::vec2(*)(const math::vec2&, float)>(&math::operator*)
		),
		sol::meta_function::division, sol::overload(
			static_cast<math::vec2(*)(const math::vec2&, const math::vec2&)>(math::operator/),
			static_cast<math::vec2(*)(const math::vec2&, float)>(&math::operator/)
		),
		sol::meta_function::equal_to, static_cast<bool(*)(const math::vec2&, const math::vec2&)>(&math::operator==),
		sol::meta_function::unary_minus, static_cast<math::vec2(*)(const math::vec2&)>(&math::operator-),
		sol::meta_function::to_string, &math::vec2::to_string
		);
	t["dot"] = &math::dot<float>;
	t["magnitude"] = &math::magnitude<float>;
	t["distance"] = &math::distance<float>;
	t["lerp"] = sol::overload(&math::lerp<float, float>, &math::lerp<math::vec2, float>);
	t.new_usertype<math::transform>("transform",
		"position", &math::transform::position,
		"scale", &math::transform::scale);
	t.new_usertype<math::rect>("rect",
		sol::call_constructor, sol::constructors<math::rect(),
			math::rect(const math::rect&),
			math::rect(const math::vec2&, const math::vec2&),
			math::rect(float, float, float, float)>(),
		"x", &math::rect::x,
		"y", &math::rect::y,
		"width", &math::rect::width,
		"height", &math::rect::height,
		"position", &math::rect::position,
		"size", &math::rect::size);
}

class b2_quick_query_callback :
	public b2QueryCallback
{
public:
	virtual bool ReportFixture(b2Fixture* fixture) override
	{
		has_collision = true;
		return false;
	}

	bool has_collision = false;
};

void script_engine::register_physics_api(physics::physics_world& pPhysics, core::scene& pScene)
{
	state["physics_raycast"] = [&pPhysics, &pScene](sol::table pResult, const math::vec2& pA, const math::vec2& pB) -> bool
	{
		const physics::raycast_hit_info hit = pPhysics.raycast_closest(pA, pB);
		if (hit.hit && pResult.valid())
		{
			pResult["normal"] = hit.normal;
			pResult["point"] = hit.point;
			if (auto state_comp = pScene.get_component<scripting::event_state_component>(hit.object_id))
				pResult["object"] = state_comp->environment;
		}
		return hit.hit;
	};

	state["physics_raycast_each"] = [this, &pPhysics, &pScene](sol::safe_function pCallable, const math::vec2& pA, const math::vec2& pB) -> bool
	{
		bool hit_once = false;
		sol::table result_info = state.create_table();
		pPhysics.raycast(
			[this, &pCallable, &pScene, &hit_once, &result_info](const physics::raycast_hit_info& hit) {
				hit_once = true;
				result_info["normal"] = hit.normal;
				result_info["point"] = hit.point;
				if (auto state_comp = pScene.get_component<scripting::event_state_component>(hit.object_id))
					result_info["object"] = state_comp->environment;
				sol::protected_function_result cont = pCallable.call(result_info);
				return cont.valid() && cont.get<bool>();
			}, pA, pB);
		return hit_once;
	};

	state["physics_test_aabb"] = [this, &pPhysics](const math::vec2& pA, const math::vec2& pB) -> bool
	{
		return pPhysics.test_aabb({ pA, pB });
	};
}

void script_engine::event_create(core::layer& pLayer)
{
	// Setup the environments if needed
	for (auto& [id, state] : pLayer.each<event_state_component>())
	{
		if (!state.environment.valid())
		{
			state.environment = create_object_environment(pLayer.get_object(id));
		}
	}

	// Event: Create
	for (auto& [id, on_create, state] :
		pLayer.each<event_selector::create, event_state_component>())
	{
		auto created = state.environment["_created"];
		if (created == false)
		{
			created = true;
			run_script(on_create.source_script, state.environment, "Create", id);
		}
	}
	pLayer.destroy_queued_components();
}

void script_engine::event_unique_create(core::layer& pLayer)
{
	// Event: Unique Create
	for (auto& [id, on_create, state] :
		pLayer.each<event_selector::unique_create, event_state_component>())
	{
		auto created = state.environment["_unique_created"];
		if (created == false)
		{
			created = true;
			run_script(on_create.source_script, state.environment, "Unique Create", id);
		}
	}
	pLayer.destroy_queued_components();
}

void script_engine::event_preupdate(core::layer& pLayer)
{
	// Event: Pre-Update
	for (auto& [id, on_preupdate, state] :
		pLayer.each<event_selector::preupdate, event_state_component>())
	{
		run_script(on_preupdate.source_script, state.environment, "Preupdate", id);
	}
	pLayer.destroy_queued_components();
}

void script_engine::event_alarms(core::layer& pLayer, std::size_t pIndex)
{
	assert(pIndex < 8);
	const std::size_t lua_index = pIndex;
	const auto delta = state["delta"].get<float>();

	// Event: Alarm {}
	for (auto& [id, on_alarm, state] :
		pLayer.each<event_component, event_state_component>({
			core::bucket_select<event_component>{ event_selector::alarm_1::bucket + pIndex } }))
	{
		auto alarm_list = state.environment["alarm"];
		auto alarm = alarm_list[lua_index];
		float alarm_timer = alarm.get<float>();
		if (alarm_timer > 0)
		{
			alarm_timer -= delta;
			if (alarm_timer <= 0)
			{
				alarm = 0;
				run_script(on_alarm.source_script, state.environment, fmt::format("Alarm {}", lua_index), id);
			}
			else
			{
				alarm = alarm_timer;
			}
		}
	}
	pLayer.destroy_queued_components();
}

void script_engine::event_update(core::layer& pLayer)
{
	// Event: Update
	for (auto& [id, on_update, state] :
		pLayer.each<event_selector::update, event_state_component>())
	{
		run_script(on_update.source_script, state.environment, "Update", id);
	}
	pLayer.destroy_queued_components();
}

void script_engine::event_postupdate(core::layer& pLayer)
{
	// Event: Post-Update
	for (auto& [id, on_postupdate, state] :
		pLayer.each<event_selector::postupdate, event_state_component>())
	{
		run_script(on_postupdate.source_script, state.environment, "Postupdate", id);
	}
	pLayer.destroy_queued_components();
}

void script_engine::event_draw(core::layer& pLayer, float pDelta)
{
	// Event: Draw
	for (auto& [id, on_create, state] :
		pLayer.each<event_selector::create, event_state_component>())
	{
		run_script(on_create.source_script, state.environment, "Draw", id);
	}
	pLayer.destroy_queued_components();
}

void script_engine::reset_object(const core::object& pObj) noexcept
{
	mObject_errors.erase(pObj.get_id());
	mRuntime_errors.erase(pObj.get_id());
	if (auto state_comp = pObj.get_component<event_state_component>())
	{
		if (!state_comp->environment.valid())
			return;
		state_comp->environment.clear();
		// Create a new environment (in-place)
		create_object_environment(pObj, state_comp->environment);
	}
}

void script_engine::run_script(script::handle& pSource, const sol::environment& pEnv, const std::string& pEvent_name, const core::object_id& pId)
{
	if (!pSource.is_valid())
		return;
	// Do not execute scripts from errornous objects.
	if (has_object_error(pId))
		return;
	// Ignore errornous scripts.
	if (has_compile_error(pSource.get_id()))
		return;
	try
	{
		// Compile script (if it can)
		if (!compile_script(pSource, pEvent_name))
			mObject_errors.insert(pId);

		auto& source = *pSource;
		if (source.function.valid())
		{
			// Execute for this objects environment.
			sol::set_environment(pEnv, source.function);
			sol::protected_function_result result = source.function();
			if (!result.valid())
			{
				sol::error err = result;
				auto& runtime_error = mRuntime_errors[pId];
				runtime_error.asset_id = pSource.get_id();
				static_cast<error_info&>(runtime_error) = parse_lua_error(err.what());
				mObject_errors.insert(pId);
				//log::error("Runtime error: {}", err.what());
				auto error_info = parse_lua_runtime_error(err.what());
				log::error("Runtime Error: {}", error_info.message);
				log::error("Stacktrace:");
				for (auto& i : error_info.stack)
				{
					auto source_info = parse_source_id(i.source);
					log::error("'{}' : {} : in {}", source_info.first, i.line, i.what)
						.in_file(i.source)
						.at_line(i.line)
						.with_userdata(util::uuid{ source_info.second });
				}
				return;
			}
		}
	}
	catch (const sol::error& e)
	{
		log::error("An unexpected error has occurred: {}", e.what());
	}
}

std::string make_valid_identifier(std::string_view pStr, const std::string_view pDefault)
{
	std::string result(pStr);
	for (auto& i : result)
	{
		// Replace all characters that are not valid with "_"
		if (!(i >= 'a' && i <= 'z' ||
			i >= 'A' && i <= 'Z' ||
			i >= '0' && i <= '9' ||
			i == '_'))
			i = '_';
	}
	// If the first character is a digit, replace it with "_"
	if (!result.empty() && result[0] >= '0' && result[0] <= '9')
		result[0] = '_';
	return result;
}

} // namespace wge::scripting
