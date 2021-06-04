#pragma once
#include <string>
#include <vector>

#include "angelscript.h"

std::string dumpBytecode(asIScriptFunction* func);
std::string dumpModule(asIScriptModule* module, const std::vector<std::string>& dependencies);
