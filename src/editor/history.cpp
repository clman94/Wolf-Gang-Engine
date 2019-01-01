#include "history.hpp"

namespace wge::editor
{

void history::add(command::ptr pCommand)
{
	mRedo.clear();
	mUndo.push_back(pCommand);
	on_change();
}

void history::execute(command::ptr pCommand)
{
	pCommand->execute();
	add(pCommand);
}

bool history::can_undo() const noexcept
{
	return !mUndo.empty();
}

bool history::can_redo() const noexcept
{
	return !mRedo.empty();
}

void history::undo()
{
	mUndo.back()->on_undo();
	mRedo.emplace_back(std::move(mUndo.back()));
	mUndo.pop_back();
	on_change();
}

void history::redo()
{
	mRedo.back()->on_redo();
	mUndo.emplace_back(std::move(mRedo.back()));
	mRedo.pop_back();
	on_change();
}

} // namespace wge::editor