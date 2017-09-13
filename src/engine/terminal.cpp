#include <cassert>
#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <functional>
#include <algorithm>
#include <map>

#include <engine/terminal.hpp>

using namespace engine;

static bool is_whitespace(char c)
{
	switch (c)
	{
	case ' ':
	case '\t':
	case '\r':
	case '\n':
		return true;
	}
	return false;
}

static bool compare_partial_string(const std::string& pStr1, const std::string& pStr2)
{
	// Exact
	if (pStr1 == pStr2)
		return true;

	// Partial
	else if (pStr1.size() > pStr2.size()
		&& pStr1.substr(0, pStr2.size()) == pStr2)
		return true;

	return false;
}

static std::vector<std::string> split_string(const std::string& pString)
{
	std::vector<std::string> retval;
	std::string segment;
	bool accept_space = false;
	for (auto i : pString)
	{
		if (i == '`')
		{
			accept_space = !accept_space; // Toggle for each grave accent
			continue;
		}

		// Check for white-space (ignore when whitespace is allowed)
		if (is_whitespace(i) && !accept_space)
		{
			// Add the segment to the return if it contains data
			// and empty the string for a new segment
			if (!segment.empty())
			{
				retval.push_back(std::move(segment));
				continue;
			}
		}

		// Add character
		segment += i;
	}
	// Push the last segment (if it exists)
	if (!segment.empty())
		retval.push_back(std::move(segment));
	return retval;
}

bool terminal_system::execute(const std::string & pCommand)
{
	const auto split = split_string(pCommand);
	if (split.empty())
		return false;
	const std::string command(split[0]); // Command is always the first one
	const terminal_arglist args(split.begin() + 1, split.end()); // The rest are the arguments
	for (size_t i = 0; i < pGroups.size(); i++)
	{
		if (pGroups[i].expired()) // Cleanup expired groups
		{
			pGroups.erase(pGroups.begin() + i);
			--i;
			continue;
		}

		std::shared_ptr<terminal_command_group> group(pGroups[i]);
		if (group->is_enabled())
		{
			// No subcommand
			if (group->get_root_command().empty())
			{
				terminal_function func = group->find_command(command);
				if (func)
					return func(args);
			}

			// Group requires the input of a subcommand
			else if (args.size() >= 1
				&& command == group->get_root_command())
			{
				const std::string sub_command(args[0]);
				const terminal_arglist sub_args(args.begin() + 1, args.end());
				terminal_function func = group->find_command(sub_command);
				if (func)
					return func(sub_args);
			}
		}
	}
	return false;
}

void terminal_system::add_group(std::shared_ptr<terminal_command_group> pGroup)
{
	pGroups.push_back(pGroup);
}

std::string terminal_system::generate_help() const
{
	std::string retval;
	for (auto& i : pGroups)
	{
		std::shared_ptr<terminal_command_group> group(i);
		if (group->is_enabled())
			retval += group->generate_help();
		else
			retval += "[N/A] " + group->generate_help();
	}
	return retval;
}

std::vector<std::string> engine::terminal_system::autocomplete(const std::string & pStr) const
{
	std::vector<std::string> ret;

	const std::vector<std::string> params = split_string(pStr);
	if (params.empty())
		return{};

	for (auto& i : pGroups)
	{
		std::shared_ptr<terminal_command_group> group(i);

		// Autocomplete global group
		if (group->get_root_command().empty())
		{
			std::vector<std::string> group_hits = group->autocomplete(params[0]);
			for (auto& j : group_hits)
				ret.push_back(group->get_root_command() + " " + j);
		}
		else
		{
			// Autocomplete root command
			if (params.size() == 1
				&& compare_partial_string(group->get_root_command(), params[0]))
				ret.push_back(group->get_root_command());

			// Group matches, check subcommand
			if (group->get_root_command() == params[0])
			{
				// Autocomplete subcommand 
				if (params.size() == 2)
				{
					std::vector<std::string> group_hits = group->autocomplete(params[1]);
					for (auto& j : group_hits)
						ret.push_back(group->get_root_command() + " " + j);
				}

				// List all sub commands
				else if (pStr.back() == ' ')
				{
					for (auto& j : group->get_list())
						ret.push_back(group->get_root_command() + " " + j);
				}
			}
		}
	}
	std::sort(ret.begin(), ret.end());
	return ret;
}


void terminal_command_group::add_command(const std::string & pCommand, terminal_function pFunction, const std::string& pHelp)
{
	// TODO: possibly check if pName has whitespace

	// Replace if command already exists
	for (auto& i : mCommand_entries)
	{
		if (i.command == pCommand)
		{
			i.function = pFunction;
			return;
		}
	}

	// Create new one
	entry nentry;
	nentry.command = pCommand;
	nentry.function = pFunction;
	nentry.help = pHelp;
	mCommand_entries.push_back(nentry);
}

bool terminal_command_group::remove_command(const std::string & pCommand)
{
	for (size_t i = 0; i < mCommand_entries.size(); i++)
	{
		if (mCommand_entries[i].command == pCommand)
		{
			mCommand_entries.erase(mCommand_entries.begin() + i);
			return true;
		}
	}
	return false;
}

terminal_function terminal_command_group::find_command(const std::string & pCommand) const
{
	for (const auto& i : mCommand_entries)
	{
		if (i.command == pCommand)
			return i.function;
	}
	return{};
}

void terminal_command_group::set_enabled(bool pIs_enabled)
{
	mIs_enabled = pIs_enabled;
}

bool terminal_command_group::is_enabled() const
{
	return mIs_enabled;
}

void engine::terminal_command_group::set_root_command(const std::string & pCommand)
{
	mRoot_command = pCommand;
}

const std::string & engine::terminal_command_group::get_root_command() const
{
	return mRoot_command;
}

std::string terminal_command_group::generate_help() const
{
	std::string retval;
	const bool has_root = !mRoot_command.empty();
	
	if (has_root)
		retval += mRoot_command + "\n";

	for (auto& i : mCommand_entries)
	{
		if (has_root)
			retval += "  ";

		retval += i.command;

		if (!i.help.empty())
			retval += " " + i.help;

		retval += "\n";
	}
	return retval;
}

std::string terminal_command_group::generate_help(std::string pCommand) const
{
	std::string retval;
	const bool has_root = !mRoot_command.empty();

	for (const auto& i : mCommand_entries)
	{
		if (i.command == pCommand)
		{
			if (has_root)
				retval += mRoot_command + " "; // "[Root]"

			retval += i.command; // "[Root] <Command>"

			if (!i.help.empty())
				retval += " " + i.help; // [Root] <Command> [Help]"

			retval += "\n";
		}
	}
	return retval;
}

std::vector<std::string> terminal_command_group::autocomplete(const std::string & pStr) const
{
	std::vector<std::string> ret;
	for (auto& i : mCommand_entries)
	{
		if (compare_partial_string(i.command, pStr))
			ret.push_back(i.command);
	}
	return ret;
}

std::vector<std::string> engine::terminal_command_group::get_list() const
{
	std::vector<std::string> ret;
	for (auto& i : mCommand_entries)
		ret.push_back(i.command);
	return ret;
}

terminal_argument::terminal_argument()
{
}

terminal_argument::terminal_argument(const std::string & pRaw)
{
	mRaw = pRaw;
}

void terminal_argument::set_raw(const std::string & pRaw)
{
	mRaw = pRaw;
}

const std::string & terminal_argument::get_raw() const
{
	return mRaw;
}

terminal_argument::operator const std::string&() const
{
	return mRaw;
}
