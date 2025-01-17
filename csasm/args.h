#pragma once

#include <string>
#include <boost/filesystem.hpp>

struct CommandlineArgs
{
	bool valid;
	
	boost::filesystem::path rootFolder;
	std::string modulePath;

	std::string binaryOutputFile;
	std::string outputFile;
	std::string dumpFile;
	
	bool verbose;
};

CommandlineArgs parseArgs(int argc, char** argv);
