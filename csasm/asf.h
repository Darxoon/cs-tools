#pragma once

#include "angelscript.h"

#include <vector>
#include <map>
#include <boost/filesystem.hpp>

typedef unsigned char uint8_t;

class BinaryCodeStream : public asIBinaryStream
{
public:
	BinaryCodeStream(const std::vector<uint8_t> &data)
		: mData(data)
	{
		
	}

	virtual void Read(void *ptr, asUINT size)
	{
		memcpy(ptr, mData.data() + mHead, size);
		mHead += size;
	}

	virtual void Write(const void *ptr, asUINT size)
	{
		const uint8_t *data = static_cast<const uint8_t *>(ptr);
		mData.insert(mData.begin(), data, data + size);
	}

private:
	std::vector<uint8_t> mData;
	int mHead = 0;
};

class AsfModuleTracker;

class AsfModule
{
public:
	AsfModule(const std::string &name, const std::vector<uint8_t> &buffer, AsfModuleTracker *tracker, bool verbose = false);
	
	asIScriptModule *getScriptModule() const
	{
		return mModule;
	}

private:
	AsfModuleTracker *mTracker;

	std::string mName;
	std::vector<uint8_t> mData;

	std::vector<std::string> mDependencies;
	asIScriptModule *mModule = nullptr;

	friend class AsfModuleTracker;
	const uint8_t cAsfMagic[4] = {
		'A', 'S', 'F', 0x01
	};
};

class AsfModuleTracker
{
public:
	AsfModuleTracker(asIScriptEngine *engine, const std::string &root);
	~AsfModuleTracker();

	asIScriptEngine *getEngine() const
	{
		return mEngine;
	}

	AsfModule *getModule(const std::string &name, bool verbose = false);

private:
	asIScriptEngine *mEngine;
	boost::filesystem::path mRoot;
	std::map<std::string, AsfModule *> mModules;
};