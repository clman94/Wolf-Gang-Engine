
#ifndef RPG_SCRIPT_CONTEXT_HPP
#define RPG_SCRIPT_CONTEXT_HPP

#include <string>

#include <engine/utility.hpp>
#include <engine/time.hpp>
#include <engine/resource.hpp>

#include <rpg/script_system.hpp>
#include <rpg/collision_box.hpp>
#include <engine/resource_pack.hpp>

#include <memory>

namespace AS = AngelScript;

namespace rpg{


static const std::string scene_script_context_restype = "script";

class scene_script_context :
	public engine::resource
{
public:

	struct wall_group_function
	{
		std::string group;
		std::shared_ptr<script_function> function;
	};

	scene_script_context();
	~scene_script_context();

	// TO BE IMPLEMENTED AS A RESOURCE
	//void set_path(const std::string& pFilepath);
	bool load() override { return true; }
	bool unload() override { return true; }

	void set_script_system(script_system& pScript);
	bool build_script(const std::string& pPath, const engine::fs::path& pData_path);
	bool build_script(const std::string& pPath, engine::resource_pack& pPack);
	bool is_valid() const;
	void clean();
	void call_all_with_tag(const std::string& pTag);

	const std::string& get_type() const override
	{
		return scene_script_context_restype;
	}

	std::vector<std::shared_ptr<script_function>> get_all_with_tag(const std::string& pTag);

	void clear_globals();

	const std::vector<wall_group_function>& get_wall_group_functions() const;

private:
	std::string mScript_path;

	// Constructs all triggers/buttons defined by functions
	// with metadata "trigger" and "button"
	// TODO: Stablize parsing of metadata
	void parse_wall_group_functions();

	util::optional_pointer<script_system> mScript;
	util::optional_pointer<AS::asIScriptModule> mScene_module;
	AS::CScriptBuilder mBuilder;

	std::vector<wall_group_function> mWall_group_functions;

	std::map<std::string, std::shared_ptr<script_function>> mTrigger_functions;

	static std::string get_metadata_type(const std::string &pMetadata);

	friend class script_system;
};

}
#endif // !RPG_SCRIPT_CONTEXT_HPP
