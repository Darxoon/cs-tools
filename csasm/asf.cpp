#include "asf.h"

#include <iostream>

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

AsfModule *AsfModuleTracker::getModule(const std::string &name, bool verbose)
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
		filePath.concat(name);
		std::ifstream inputStream(filePath.string(), std::ios::binary);
		std::vector<uint8_t> inputBuffer((std::istreambuf_iterator<char>(inputStream)), (std::istreambuf_iterator<char>()));
		auto *newModule = new AsfModule(name, inputBuffer, this, verbose);
		mModules[name] = newModule;
		return newModule;
	}
}

AsfModule::AsfModule(const std::string &name, const std::vector<uint8_t> &buffer, AsfModuleTracker *tracker, bool verbose)
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
	for (const auto &dep : mDependencies)
	{
		if (!mTracker->getModule(dep, verbose))
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
