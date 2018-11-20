#pragma once

#include <memory>

namespace wge::editor
{

class command
{
public:
	using ptr = std::shared_ptr<command>;

	virtual void execute() = 0;
	virtual void on_undo() = 0;
	virtual void on_redo() = 0;
};

} // namespace wge::editor
