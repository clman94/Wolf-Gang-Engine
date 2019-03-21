#include <wge/util/uuid_rerouter.hpp>

namespace wge::util
{

uuid uuid_rerouter::route(const uuid& pFrom)
{
	// Check if a redirect already exists
	uuid result = find_redirect(pFrom);
	if (!result.is_valid())
		return result;

	// Generate a new uuid and redirect
	// the original uuid to it.
	result = generate_uuid();
	redirect(pFrom, result);
	return result;
}

void uuid_rerouter::bypass(const uuid& pOriginal)
{
	redirect(pOriginal, pOriginal);
}

void uuid_rerouter::redirect(const uuid& pFrom, const uuid& pTo)
{
	mEntries.push_back(entry{ pFrom, pTo });
}

uuid uuid_rerouter::find_redirect(const uuid& pFrom) const
{
	for (auto& i : mEntries)
		if (i.from == pFrom)
			return i.to;
	return{};
}

} // namespace wge::util
