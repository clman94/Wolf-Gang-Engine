#include "history.hpp"

namespace wge::editor
{

void history::add(command::uptr&& pCommand)
{
	mRedo.clear();
	mUndo.emplace_back(std::forward<command::uptr>(pCommand));
	on_change();
}

void history::execute(command::uptr&& pCommand)
{
	pCommand->execute();
	add(std::forward<command::uptr>(pCommand));
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