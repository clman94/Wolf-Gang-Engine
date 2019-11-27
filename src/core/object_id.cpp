#include <wge/core/object_id.hpp>

namespace wge::core
{

static object_id_generator gObject_id_generator;

object_id_generator& get_global_generator()
{
	return gObject_id_generator;
}

} // namespace wge::core
