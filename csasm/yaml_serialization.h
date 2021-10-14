#pragma once

#include <string>
#include <vector>

#include "angelscript.h"

//std::string serializeBytecode(asIScriptFunction* func);
std::string serializeModuleYaml(asIScriptModule* module, const std::vector<std::string>& dependencies);
