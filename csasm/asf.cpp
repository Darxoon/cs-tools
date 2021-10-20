#include "asf.h"

#include <iostream>

file_not_found::file_not_found(std::string filepath, std::string role) : m_filepath(filepath), role(role), msg((role.empty() ? "F" : role + " f") + "ile not found: '" + filepath + "'")
{
	
}

const char* file_not_found::what() const
{
	return msg.c_str();
}


AsfModuleTracker::AsfModuleTracker(asIScriptEngine *engine, const std::string &root)
{
	mEngine = engine;
	mRoot = root;
}

AsfModuleTracker::~AsfModuleTracker()
{
	for (const auto &it : mModules)
	{
		delete it.second;
	}
}

AsfModule *AsfModuleTracker::getModule(const std::string &name, std::vector<std::string>* direct_dependencies, bool verbose)
{
	// Check if it has been loaded already
	auto it = mModules.find(name);
	if (it != mModules.end())
	{
		return it->second;
	}
	else
	{
		// Not found, load
		boost::filesystem::path filePath = mRoot;
		
		char last_char = filePath.string()[filePath.string().length() - 1];
		if (last_char != '/' && last_char != '\\')
		{
			filePath.concat("/");
		}
		
		filePath.concat(name);

		if (!exists(filePath))
		{
			throw file_not_found(filePath.string(), "");
		}
		
		std::ifstream inputStream(filePath.string(), std::ios::binary);
		std::vector<uint8_t> inputBuffer((std::istreambuf_iterator<char>(inputStream)), (std::istreambuf_iterator<char>()));
		auto *newModule = new AsfModule(name, inputBuffer, this, direct_dependencies, verbose);
		mModules[name] = newModule;
		return newModule;
	}
}

AsfModule::AsfModule(const std::string &name, const std::vector<uint8_t> &buffer, AsfModuleTracker *tracker, std::vector<std::string>* direct_dependencies, bool verbose)
	: mName(name), mData(buffer), mTracker(tracker)
{
	mName = name;
	mData = buffer;
	
	// Parse ASF header
	uint8_t *data = mData.data();
	if (memcmp(data, cAsfMagic, sizeof(cAsfMagic)))
	{
		return;
	}
	data += sizeof(cAsfMagic);
	uint32_t codeOffset = *reinterpret_cast<uint32_t *>(data);
	data += sizeof(uint32_t);
	uint32_t codeSize = *reinterpret_cast<uint32_t *>(data);
	data += sizeof(uint32_t);
	uint32_t dependencyCount = *reinterpret_cast<uint32_t *>(data);
	data += sizeof(uint32_t);

	// Parse dependencies
	for (uint32_t i = 0; i < dependencyCount; ++i)
	{
		mDependencies.emplace_back(reinterpret_cast<char *>(data));
		data += 128;
	}

	// Load dependencies
	if (verbose)
	{
		for (const auto& dep : mDependencies)
		{
			std::cout << "Requires dependency: " << dep << std::endl;
		}
		std::cout << std::endl;
	}
	if (direct_dependencies != nullptr)
	{
		direct_dependencies->reserve(direct_dependencies->size() + mDependencies.size());
	}
	for (const auto &dep : mDependencies)
	{
		if (direct_dependencies != nullptr)
		{
			direct_dependencies->emplace_back(std::string(dep));
		}
		if (!mTracker->getModule(dep, nullptr, verbose))
		{
			// Dependency failed to load
			return;
		}
	}

	// Load code
	BinaryCodeStream code(std::vector<uint8_t>(mData.begin() + codeOffset, 
											   mData.begin() + codeOffset + codeSize));
	mModule = mTracker->getEngine()->GetModule(name.c_str(), asGM_ALWAYS_CREATE);

	bool debugInfo = false;
	mModule->LoadByteCode(&code, &debugInfo);
}

std::vector<uint8_t> AsfModule::save() const
{
	// Get bytecode
	BinaryCodeStream codeStream{ std::vector<uint8_t>() };
	std::cout << mModule->SaveByteCode(&codeStream) << std::endl;

	std::vector<uint8_t> bytecode = codeStream.getData();
	
	// Construct Asf File
	int codeOffset = 0x2010; // Code offset always starts at 0x2010
	int codeSize = bytecode.size();
	int dependencyCount = mDependencies.size();
	
	std::vector<uint8_t> out;
	out.reserve(codeOffset + codeSize);
	
	// Write ASF header
	out.insert(out.end(), cAsfMagic, cAsfMagic + 4);

	out.insert(out.end(), reinterpret_cast<uint8_t*>(&codeOffset), reinterpret_cast<uint8_t*>(&codeOffset + 1));
	
	out.insert(out.end(), reinterpret_cast<uint8_t*>(&codeSize), reinterpret_cast<uint8_t*>(&codeSize + 1));

	out.insert(out.end(), reinterpret_cast<uint8_t*>(&dependencyCount), reinterpret_cast<uint8_t*>(&dependencyCount + 1));

	// Write dependencies
	for (const std::string& dependency : mDependencies)
	{
		out.insert(out.end(), dependency.begin(), dependency.end());
		
		for (int i = 0; i < 128 - dependency.size(); ++i)
		{
			out.push_back(0);
		}
	}

	// To replicate original ASF files, pad with zeros until the length is 0x2010
	// I have no idea why this is, but in every ASF file the code offset starts at offset 0x2010
	if (out.size() > codeOffset)
		throw std::exception("Dependency list too large");

	while (out.size() < codeOffset)
		out.push_back(0);


	out.insert(out.end(), bytecode.begin(), bytecode.end());

	return out;
}
