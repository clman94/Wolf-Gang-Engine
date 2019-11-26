#include <wge/core/asset.hpp>
#include <wge/core/game_object.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/math/transform.hpp>

namespace wge::core
{

class scene;

class scene_resource :
	public resource
{
public:
	struct instance
	{
		// Unique name of instance.
		std::string name;
		math::transform transform;
		object_id id;
		util::uuid asset_id;
	};

	std::vector<instance> instances;

public:
	virtual json serialize_data() const override;
	virtual void deserialize_data(const json& pJson) override;

	static core::object generate_instance(core::layer& pLayer, const core::asset_manager& pAsset_mgr, const instance& pData);
	void generate_layer(core::layer& pLayer, const core::asset_manager& pAsset_mgr) const;
	void generate_scene(core::scene& pScene, const core::asset_manager& pAsset_mgr) const;
};

} // namespace wge::core
