#include <wge/scripting/lua_engine.hpp>
#include <wge/physics/physics_world.hpp>
#include <Box2D/Dynamics/b2World.h>

namespace wge::scripting
{

lua_engine::lua_engine()
{
	register_api();
}

lua_engine::~lua_engine()
{
	// Clear the environment first to prevent hanging references
	global_environment = sol::environment{};
}

void lua_engine::execute_global_scripts(core::asset_manager& pAsset_manager)
{
	// Reset the global environment
	global_environment = sol::environment(state, sol::create, state.globals());

	// Run all global scripts
	for (auto& i : pAsset_manager.get_asset_list())
	{
		if (i->get_type() == "script")
		{
			auto res = i->get_resource<script>();
			try
			{
				state.safe_script(res->source, global_environment, pAsset_manager.get_asset_path(i).string(), sol::load_mode::text);
			}
			catch (sol::error& e)
			{
				log::error() << e.what() << log::endm;
			}
		}
	}
}

sol::environment lua_engine::create_object_environment(const core::game_object& pObj)
{
	sol::environment env(state, sol::create, global_environment);

	env["this"] = env;

	auto get_position = [pObj]() -> math::vec2
	{
		auto transform = pObj.get_component<core::transform_component>();
		return transform->get_position();
	};
	auto set_position = [pObj](const math::vec2& pPos)
	{
		auto transform = pObj.get_component<core::transform_component>();
		transform->set_position(pPos);
	};

	//env["position"] = sol::property(get_position, set_position);
	env["get_position"] = get_position;
	env["set_position"] = set_position;
	env["move"] = [get_position, set_position](const math::vec2& pDirection)
	{
		set_position(get_position() + pDirection);
	};

	env["this_layer"] = std::ref(pObj.get_layer());

	return env;
}

void lua_engine::update_delta(float pSeconds)
{
	state["delta"] = pSeconds;
}

bool lua_engine::compile_script(script& pScript)
{
	sol::load_result result = state.load(pScript.source, pScript.get_source_path().string());
	if (!result.valid())
		return false;
	pScript.function = result.get<sol::protected_function>();
	return true;
}

void lua_engine::register_api()
{
	state.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string, sol::lib::table);
	state["table"]["haskey"] = [](sol::table pTable, sol::object pKey) -> bool
	{
		for (auto& i : pTable)
			if (i.second != sol::nil && i.first == pKey)
				return true;
		return false;
	};

	state["delta"] = 1.f;
	state["dprint"] = sol::overload(
		[](const std::string& pVal) { log::debug() << pVal << log::endm; },
		[](double pVal) { log::debug() << pVal << log::endm; }
	);

	register_math_api();
	register_physics_api();
}

void lua_engine::register_math_api()
{
	sol::table t = state.create_named_table("math");
	t["pi"] = math::pi;
	t["sin"] = [](float pRad) { return math::sin(pRad); };
	t["cos"] = [](float pRad) { return math::cos(pRad); };
	t["abs"] = &math::abs<float>;
	t["floor"] = &math::floor<float>;
	t["round"] = &math::round<float>;
	t["max"] = &math::max<float>;
	t["min"] = &math::min<float>;
	t["clamp"] = &math::clamp<float>;
	t["sqrt"] = &math::sqrt<float>;
	t["mod"] = &math::mod<float>;
	t["pow"] = &math::pow<float>;
	t.new_usertype<math::vec2>("vec2",
		sol::call_constructor, sol::constructors<math::vec2(), math::vec2(float, float), math::vec2(const math::vec2&)>(),
		"x", &math::vec2::x,
		"y", &math::vec2::y,
		sol::meta_function::addition, &math::vec2::operator+,
		sol::meta_function::subtraction, static_cast<math::vec2(math::vec2::*)(const math::vec2&) const>(&math::vec2::operator-),
		sol::meta_function::multiplication, sol::overload(
			static_cast<math::vec2(math::vec2::*)(const math::vec2&) const>(&math::vec2::operator*),
			static_cast<math::vec2(math::vec2::*)(float) const>(&math::vec2::operator*)
		),
		sol::meta_function::division, static_cast<math::vec2(math::vec2::*)(const math::vec2&) const>(&math::vec2::operator/),
		sol::meta_function::equal_to, static_cast<bool(math::vec2::*)(const math::vec2&) const>(&math::vec2::operator==),
		sol::meta_function::to_string, &math::vec2::to_string,
		sol::meta_function::unary_minus, static_cast<math::vec2(math::vec2::*)() const>(&math::vec2::operator-)
		);
	t["dot"] = &math::dot;
	t["normal"] = &math::normal<math::vec2>;
	t["magnitude"] = &math::magnitude;
	t["distance"] = &math::distance;
	t.new_usertype<math::transform>("transform",
		"position", &math::transform::position,
		"scale", &math::transform::scale);
	t.new_usertype<math::rect>("rect",
		sol::call_constructor, sol::constructors<math::rect(),
			math::rect(const math::vec2&, const math::vec2&),
			math::rect(float, float, float, float), math::rect(const math::rect&)>(),
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

void lua_engine::register_physics_api()
{
	sol::table t = state.create_named_table("physics");
	t["test_aabb"] = [this](const core::layer& pLayer, const math::vec2& a, const math::vec2& b)
	{
		if (auto world = pLayer.get_system<physics::physics_world>())
		{
			b2_quick_query_callback callback;
			b2AABB aabb;
			aabb.lowerBound.x = a.x;
			aabb.lowerBound.y = a.y;
			aabb.upperBound.x = b.x;
			aabb.upperBound.y = b.y;
			world->get_world()->QueryAABB(&callback, aabb);
			return callback.has_collision;
		}
		return false;
	};
}

void script_system::update(float pDelta)
{
	const auto run_script = [&](const std::string& pSource, const sol::environment& pEnv)
	{
		try
		{
			mLua_engine.state.safe_script(pSource, pEnv);
		}
		catch (const sol::error& e)
		{
			std::cout << "An expected error has occurred: " << e.what() << std::endl;
		}
	};

	mLua_engine.update_delta(pDelta);

	// Setup the environments if needed
	get_layer().for_each([&](core::game_object pObj, event_state_component& pState)
	{
		if (!pState.environment.valid())
		{
			pState.environment = mLua_engine.create_object_environment(pObj);

			// Added the properties as variables.
			for (const auto& i : pState.properties)
			{
				if (!i.name.empty())
				{
					std::visit([&](auto& val)
					{
						pState.environment[i.name] = val;
					}, i.value);
				}
			}
		}
	});

	// Event: Create
	get_layer().for_each([&](
		event_components::on_create& pOn_create,
		event_state_component& pState)
	{
		if (pOn_create.first_time)
		{
			pOn_create.first_time = false;
			run_script(pOn_create.source, pState.environment);
		}
	});

	// Event: Update
	get_layer().for_each([&](
		event_components::on_update& pOn_update,
		event_state_component& pState)
	{
		run_script(pOn_update.source, pState.environment);
	});
}

std::string make_valid_identifier(const std::string_view& pStr)
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
