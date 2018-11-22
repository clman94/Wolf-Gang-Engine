#pragma once

namespace wge::editor
{

class inspector
{
	virtual bool can_inspect()
	{
		return false;
	}

};

} // namespace wge::editor
