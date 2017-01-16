#ifndef RPG_CONTROLS_HPP
#define RPG_CONTROLS_HPP

namespace rpg {

class controls
{
public:
	enum class control
	{
		activate,
		left,
		right,
		up,
		down,
		select_next,
		select_previous,
		select_up,
		select_down,
		back,
		menu,
		reset,
		reset_game,
		editor_1,
		editor_2,
	};
	controls();
	void trigger(control pControl);
	bool is_triggered(control pControl);
	void reset();

	void update(engine::renderer& pR);

private:
	std::array<bool, 15> mControls;
};

}
#endif // !RPG_CONTROLS_HPP
