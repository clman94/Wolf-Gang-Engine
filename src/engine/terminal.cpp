#include <cassert>
#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <functional>
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
			else if (args.size() >= 1)
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

void terminal_command_group::add_command(const std::string & pCommand, terminal_function pFunction)
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

	// Ccreate new one
	entry nentry;
	nentry.command = pCommand;
	nentry.function = pFunction;
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
