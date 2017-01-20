
#ifndef RPG_SCRIPT_CONTEXT_HPP
#define RPG_SCRIPT_CONTEXT_HPP

#include <string>

#include <engine/utility.hpp>
#include <engine/time.hpp>

#include <rpg/script_system.hpp>
#include <rpg/collision_box.hpp>

#include <memory>

namespace AS = AngelScript;

namespace rpg{

class script_context
{
public:
	script_context();
	~script_context();

	void set_script_system(script_system& pScript);
	bool build_script(const std::string& pPath);
	bool is_valid() const;
	void clean();
	void start_all_with_tag(const std::string& pTag);

	const std::vector<trigger>& get_script_defined_triggers() const;
	const std::vector<trigger>& get_script_defined_buttons() const;

private:

	// Constructs all triggers/buttons defined by functions
	// with metadata "trigger" and "button"
	// TODO: Stablize parsing of metadata
	void parse_script_defined_triggers();

	util::optional_pointer<script_system> mScript;
	util::optional_pointer<AS::asIScriptModule> mScene_module;
	AS::CScriptBuilder mBuilder;

	std::vector<trigger> mScript_defined_triggers;
	std::vector<trigger> mScript_defined_buttons;

	std::map<std::string, std::unique_ptr<script_function>> mTrigger_functions;

	static std::string get_metadata_type(const std::string &pMetadata);

	friend class script_system;
};

}
#endif // !RPG_SCRIPT_CONTEXT_HPP
