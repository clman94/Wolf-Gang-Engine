#pragma once

#include <memory>
#include <deque>

namespace wge::editor
{

class history
{
public:
	class command
	{
	public:
		using ptr = std::shared_ptr<command>;

		virtual void execute() = 0;
		virtual void on_undo() = 0;
		virtual void on_redo() = 0;
	};

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

	bool can_undo() const;
	bool can_redo() const;
	void undo();
	void redo();

private:
	std::deque<command::ptr> mUndo;
	std::deque<command::ptr> mRedo;
};


} // namespace wge::editor