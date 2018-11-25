#include <wge/core/layer.hpp>
#include <wge/core/context.hpp>

namespace wge::core
{

game_object layer::create_object()
{
	auto& obj = mObjects.emplace_back(*this);
	obj.set_instance_id(get_context().get_unique_instance_id());
	return obj;
}

} // namespace wge::core
