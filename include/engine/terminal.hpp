#ifndef ENGINE_TERMINAL_HPP
#define ENGINE_TERMINAL_HPP

#include <functional>
#include <engine/utility.hpp>

namespace engine
{

class terminal_argument
{
public:
	terminal_argument();
	terminal_argument(const std::string& pRaw);

	void set_raw(const std::string& pRaw);
	const std::string& get_raw() const;

	template<typename T>
	inline T to_number() const
	{
		return util::to_numeral<T>(mRaw);
	}

	operator const std::string&() const;
private:
	std::string mRaw;
};
typedef std::vector<terminal_argument> terminal_arglist;

typedef std::function<bool(const terminal_arglist& pArgs)> terminal_function;

template<class T, class Tret, class...Tparams>
inline std::function<Tret(Tparams...)> wrap_member_instance(Tret(T::*pFunc)(Tparams...), T* pInstance)
{
	return [=](Tparams...pArgs)->Tret
	{
		auto func = std::mem_fn(pFunc);
		return func(*pInstance, std::forward<Tparams...>(pArgs)...);
	};
}

class terminal_command_group
{
public:
	terminal_command_group()
		: mIs_enabled(true) {}

	void add_command(const std::string& pCommand, terminal_function pFunction, const std::string& pHelp = std::string());
	bool remove_command(const std::string& pCommand);
	terminal_function find_command(const std::string& pCommand) const;

	void set_enabled(bool pIs_enabled);
	bool is_enabled() const;

	void set_root_command(const std::string& pCommand);
	const std::string& get_root_command() const;

	std::string generate_help() const;
	std::string generate_help(std::string pCommand) const;

private:
	std::string mRoot_command;
	bool mIs_enabled;

	struct entry
	{
		std::string help;
		std::string command;
		terminal_function function;
	};

	std::vector<entry> mCommand_entries;
};

class terminal_system
{
public:
	bool execute(const std::string& pCommand);
	void add_group(std::shared_ptr<terminal_command_group> pGroup);

	std::string generate_help() const;

private:
	std::vector<std::weak_ptr<terminal_command_group>> pGroups;
};

}
#endif // !ENGINE_TERMINAL_HPP
