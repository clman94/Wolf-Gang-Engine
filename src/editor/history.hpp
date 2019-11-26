#pragma once

#include <wge/util/signal.hpp>

#include "command.hpp"

#include <memory>
#include <deque>
#include <variant>

namespace wge::editor
{

class history
{
public:
	// Adds a command to the undo stack.
	// Doesn't execute anything.
	void add(command::uptr&& pCommand);

	// Executes then adds the command to the undo stack.
	void execute(command::uptr&& pCommand);

	// Create, execute, and add to undo stack. Use for convenience.
	template<typename T, typename...Targs>
	void execute(Targs&&...pArgs)
	{
		add(std::make_unique<T>(std::forward<Targs>(pArgs)...));
	}

	bool can_undo() const noexcept;
	bool can_redo() const noexcept;
	void undo();
	void redo();

	util::signal<void()> on_change;

private:
	std::deque<command::uptr> mUndo;
	std::deque<command::uptr> mRedo;
};


} // namespace wge::editor

/*
namespace wge::util
{

struct other_state {};

template <typename Tstate>
class action_state
{
public:
	action_state(history& pHistory, Tstate pDefault_state) :
		mHistory(pHistory),
		mState(pDefault_state)
	{}

	void begin(Tstate mType)
	{
		mState = mType;
	}

private:
	history* mHistory;
	Tenum mState;
};

template<typename...T>
class variant_state :
	action_state<std::variant<other_state, T...>>
{
public:

};

} // namespace wge::util
*/