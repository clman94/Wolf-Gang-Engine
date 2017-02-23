
#ifndef RPG_SCRIPT_CONTEXT_HPP
#define RPG_SCRIPT_CONTEXT_HPP

#include <string>

#include <engine/utility.hpp>
#include <engine/time.hpp>
#include <engine/resource.hpp>

#include <rpg/script_system.hpp>
#include <rpg/collision_box.hpp>

#include <memory>

namespace AS = AngelScript;

namespace rpg{



class script_context :
	public engine::resource
{
public:
	struct wall_group_function
	{
		std::string group;
		script_function* function;
	};

	script_context();
	~script_context();

	void set_path(const std::string& pFilepath);
	bool load();
	bool unload();

	void set_script_system(script_system& pScript);
	bool build_script(const std::string& pPath);
	bool is_valid() const;
	void clean();
	void start_all_with_tag(const std::string& pTag);

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

	std::map<std::string, std::unique_ptr<script_function>> mTrigger_functions;

	static std::string get_metadata_type(const std::string &pMetadata);

	friend class script_system;
};

}
#endif // !RPG_SCRIPT_CONTEXT_HPP
