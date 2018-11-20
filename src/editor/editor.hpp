#pragma once

#include <memory>

namespace wge::editor
{

class editor_context;

class editor
{
public:
	using ptr = std::shared_ptr<editor>;

	virtual ~editor() {}

	// Normally called before on_gui. Update any logic here.
	virtual void on_update(float pDelta) {}
	// Called when the gui needs to be drawn. Do gui stuff here.
	virtual void on_gui(float pDelta) {}
	// Called when some outside source wants to give focus to this editor
	virtual void on_request_focus() {}
	// Closes this editor
	virtual void on_close() {}
};

} // namespace wge::editor
