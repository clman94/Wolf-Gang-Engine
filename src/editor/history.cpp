#include "history.hpp"

namespace wge::editor
{

void history::add(command::ptr pCommand)
{
	mRedo.clear();
	mUndo.push_back(pCommand);
}

void history::execute(command::ptr pCommand)
{
	pCommand->execute();
	add(pCommand);
}

bool history::can_undo() const
{
	return !mUndo.empty();
}

bool history::can_redo() const
{
	return !mRedo.empty();
}

void history::undo()
{
	mUndo.back()->on_undo();
	mRedo.emplace_back(std::move(mUndo.back()));
	mUndo.pop_back();
}

void history::redo()
{
	mRedo.back()->on_redo();
	mUndo.emplace_back(std::move(mRedo.back()));
	mRedo.pop_back();
}

} // namespace wge::editor