#include <iostream>
#include <exception>

#include <engine/renderer.hpp>
#include <engine/time.hpp>
#include <engine/logger.hpp>

#include <rpg/rpg.hpp>

#include <rpg/editor.hpp>

#ifndef TESTS

// Entry point of application
int main(int argc, char* argv[])
{
	logger::initialize("./log.txt");
	//try
	//{
		editors::WGE_editor editor;

		editor.initualize("./data");
		editor.run();
	/*}
	catch (std::exception& e)
	{
		logger::error("A main exception occurred: " + std::string(e.what()) + "\n");
		std::getchar();
	}*/
	
	return 0;
}

#endif