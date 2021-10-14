#pragma once

#include <string>
#include <boost/filesystem.hpp>

struct CommandlineArgs
{
	bool valid;
	
	boost::filesystem::path rootFolder;
	boost::filesystem::path configFile;
	std::string modulePath;

	std::string outputFile;
	std::string yamlOutputFile;
	std::string dumpFile;
	
	bool verbose;
};

CommandlineArgs parseArgs(int argc, char** argv);
