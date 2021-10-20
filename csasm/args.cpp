#include "args.h"

#include <iostream>
#include <boost/program_options.hpp>

static void print_help(const boost::program_options::options_description& description)
{
	std::cout << "Usage: csasm <data-root> <module>\n\n";
	std::cout << "Required options:\n  data-root             The romfs folder. The scripts have to be decrypted.\n"
		"  module                The relative path to the AngelScript.bin file.\n\n";
	std::cout << description << std::endl;
}

CommandlineArgs parseArgs(const int argc, char** argv)
{
	using namespace boost::program_options;
	using boost::program_options::value;

	// Declare the supported options.
	options_description visible_options("Allowed options");
	visible_options.add_options()
		("help,h", "produce help message")
		("verbose,v", "log dependencies and other debugging messages")
		("output,o", value<std::string>(), "the yaml output file. Less detailed than the dump but can be reassembled.")
		("dump,d", value<std::string>(), "the dump output file. This is the default console output. Contains slightly more information than yaml but can't be reassembled.")
		("bin,b", value<std::string>(), "the binary AngelScript output file.")
		;

	options_description hidden_options;
	hidden_options.add_options()
		("data-root", value<std::string>())
		("module", value<std::string>())
		;

	options_description total_options;
	total_options.add(visible_options).add(hidden_options);
	
	try {

		// Positional arguments
		positional_options_description p;
		p.add("data-root", 1);
		p.add("module", 1);

		variables_map map;
		store(command_line_parser(argc, argv).options(total_options).positional(p).run(), map);
		notify(map);

		// evaluate
		if (map.count("help") || argc == 1) {
			print_help(visible_options);
			return { false };
		}

		return {
			true,
			map["data-root"].as<std::string>(),
			map["module"].as<std::string>(),
			map.count("bin") ? map["bin"].as<std::string>() : "",
			map.count("output") ? map["output"].as<std::string>() : "",
			map.count("dump") ? map["dump"].as<std::string>() : "",
			map.count("verbose") > 0,
		};
		
	} catch (error& e) {
		std::cerr << "Error: " << e.what() << std::endl << std::endl;
		print_help(visible_options);
		return { false };
	}
}
