#include "args.h"

#include <iostream>
#include <boost/program_options.hpp>

CommandlineArgs parseArgs(const int argc, char** argv)
{
	namespace program_options = boost::program_options;

	// Declare the supported options.
	program_options::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message")
		("verbose,v", "log dependencies and other debugging messages")
		("data-root", program_options::value<std::string>(), "the root folder of the rom")
		("registry", program_options::value<std::string>(), "the config registry.json file")
		("module", program_options::value<std::string>(), "the relative path to the AngelScript .bin file")
		;

	// Positional arguments
	program_options::positional_options_description p;
	p.add("data-root", 1);
	p.add("registry", 1);
	p.add("module", 1);

	program_options::variables_map map;
	program_options::store(program_options::command_line_parser(argc, argv).
		options(desc).positional(p).run(), map);
	notify(map);

	// evaluate
	if (map.count("help")) {
		std::cout << "Usage: csasm <data-root> <registry> <module>\n\n" << desc;
		return { false };
	}

	return {
		true,
		map["data-root"].as<std::string>(),
		map["registry"].as<std::string>(),
		map["module"].as<std::string>(),
		map.count("verbose") > 0,
	};
}
