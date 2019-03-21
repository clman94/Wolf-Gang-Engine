#pragma once

#include "uuid.hpp"

#include <vector>

namespace wge::util
{

// A tool used to generate new uuids and redirect the originals
// in serialization.
class uuid_rerouter
{
public:
	// Redirect this id or generate a new one if it doesn't exist.
	uuid route(const uuid& pFrom);

	// Disables generating and/or redirecting for a specific id.
	// This means the original will always be used.
	void bypass(const uuid& pOriginal);

	// Replace all ids that are pFrom with pTo
	void redirect(const uuid& pFrom, const uuid& pTo);

private:
	uuid find_redirect(const uuid& pFrom) const;

private:
	struct entry
	{
		uuid from, to;
	};
	std::vector<entry> mEntries;
};

} // namespace wge::util
