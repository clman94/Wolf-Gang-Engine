#include <wge/scripting/lua_engine.hpp>

namespace wge::scripting
{

lua_engine::lua_engine()
{
	register_api();
}

lua_engine::~lua_engine()
{
	// Clear the environment first to prevent hanging references
	main_environment = sol::environment{};
}

void lua_engine::execute_global_scripts(core::asset_manager& pAsset_manager)
{
	// Reset the global environment
	main_environment = sol::environment(state, sol::create, state.globals());

	// Run all global scripts
	for (auto& i : pAsset_manager.get_asset_list())
	{
		if (i->get_type() == "script")
		{
			auto res = i->get_resource<script>();
			try
			{
				state.safe_script(res->source, main_environment, i->get_path().string(), sol::load_mode::text);
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
	sol::environment env(state, sol::create, main_environment);

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

	return env;
}

void lua_engine::update_delta(float pSeconds)
{
	state["delta"] = pSeconds;
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
}

void lua_engine::register_math_api()
{
	sol::table t_math = state.create_named_table("math");
	t_math["pi"] = math::pi;
	t_math["sin"] = [](float pRad) { return math::sin(pRad); };
	t_math["cos"] = [](float pRad) { return math::cos(pRad); };
	t_math["abs"] = &math::abs<float>;
	t_math["floor"] = &math::floor<float>;
	t_math["round"] = &math::round<float>;
	t_math["max"] = &math::max<float>;
	t_math["min"] = &math::min<float>;
	t_math["clamp"] = &math::clamp<float>;
	t_math["sqrt"] = &math::sqrt<float>;
	t_math["mod"] = &math::mod<float>;
	t_math.new_usertype<math::vec2>("vec2",
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
	t_math["dot"] = &math::dot;
	t_math["normal"] = &math::normalize<math::vec2>;
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

} // namespace wge::scripting
