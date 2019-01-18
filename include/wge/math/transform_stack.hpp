#pragma once

#include "transform.hpp"

#include <stack>

namespace wge::math
{

class transform_stack
{
public:
	transform_stack()
	{
		// Start with the identity
		mStack.push(math::transform{});
	}

	void push(const math::transform& pMat)
	{
		mStack.push(mStack.top().apply_to(pMat));
	}

	// Returns the popped transform
	math::transform pop()
	{
		if (mStack.size() > 1)
		{
			math::transform temp = mStack.top();
			mStack.pop();
			return temp;
		}
		return{};
	}

	// Get the current transform
	const math::transform& get() const noexcept
	{
		return mStack.top();
	}

	math::transform apply_to(const math::transform& pTransform) const noexcept
	{
		return mStack.top().apply_to(pTransform);
	}

	math::vec2 apply_to(const math::vec2& pVec) const noexcept
	{
		return mStack.top().apply_to(pVec);
	}

	math::vec2 apply_inverse_to(const math::vec2& pVec) const noexcept
	{
		return mStack.top().apply_inverse_to(pVec);
	}

	bool is_identity() const noexcept
	{
		// First matrix is always the identity
		return mStack.size() == 1;
	}

private:
	std::stack<math::transform> mStack;
};

} // namespace wge::math
