#pragma once

#include <string>
#include <vector>

#include "json.hpp"

#include "angelscript.h"

std::string serializeBytecode(asIScriptFunction* func);
nlohmann::json serializeModule(asIScriptModule* module, const std::vector<std::string>& dependencies);
