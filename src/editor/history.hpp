#pragma once

#include "command.hpp"
#include "signal.hpp"

#include <memory>
#include <deque>

namespace wge::editor
{

class history
{
public:
	// Adds a command to the undo stack
	void add(command::ptr pCommand);

	// Executes then adds the command to the undo stack
	void execute(command::ptr pCommand);

	// Create, execute, and add to undo stack
	template<typename T, typename...Targs>
	void execute(Targs&&...pArgs)
	{
		add(std::make_shared<T>(std::forward<Targs>(pArgs)...));
	}

	bool can_undo() const noexcept;
	bool can_redo() const noexcept;
	void undo();
	void redo();

	util::signal<void()> on_change;

private:
	std::deque<command::ptr> mUndo;
	std::deque<command::ptr> mRedo;
};


} // namespace wge::editor