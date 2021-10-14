#include "yaml_serialization.h"


#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include <boost/algorithm/string.hpp>

#include "angelscript.h"
#include <../source/as_scriptengine.h>


template<typename... A>
static std::string fmtString(const std::string& format, A... args)
{
	// Thread safe
	static thread_local char sFormatBuf[2048];
	snprintf(sFormatBuf, sizeof(sFormatBuf), format.c_str(), args...);
	return std::string(sFormatBuf);
}

static std::string serializeBytecode(asIScriptFunction* func)
{
	std::string dump;
	asCScriptFunction* function = static_cast<asCScriptFunction*>(func);

	asCScriptEngine* engine = static_cast<asCScriptEngine*>(function->GetEngine());

	asUINT length;
	asDWORD* code = function->GetByteCode(&length);
	for (uint32_t position = 0; position < length; )
	{
		std::string disas = "UNK";

		// Read opcode
		asDWORD* inst = code + position;
		asEBCInstr op = static_cast<asEBCInstr>(*reinterpret_cast<asBYTE*>(inst));
		const char* mnem = asBCInfo[op].name;
		switch (asBCInfo[op].type)
		{
		case asBCTYPE_NO_ARG: // no args
		{
			disas = fmtString("%-8s", mnem);
		}
		break;
		case asBCTYPE_W_ARG: // word imm
		{
			asWORD arg0 = asBC_WORDARG0(inst);
			switch (op)
			{
			case asBC_STR:
			{
				const auto& str = engine->GetConstantString(arg0);
				std::string formatted = str.AddressOf();
				boost::replace_all(formatted, "\\", "\\\\");
				boost::replace_all(formatted, "\n", "\\n");
				disas = fmtString("%-8s %d (%d:\"%s\")", mnem, arg0, str.GetLength(), formatted.c_str());
			}
			break;
			default:
				disas = fmtString("%-8s %d", mnem, arg0);
				break;
			}
		}
		break;
		case asBCTYPE_wW_ARG: // word var dst
		case asBCTYPE_rW_ARG: // word var src
		{
			uint16_t arg0 = asBC_WORDARG0(inst);
			disas = fmtString("%-8s v%d", mnem,
				reinterpret_cast<int16_t&>(arg0));
		}
		break;
		case asBCTYPE_wW_rW_ARG: // word dst, word src
		case asBCTYPE_rW_rW_ARG: // word src, word src
		{
			uint16_t arg0 = asBC_WORDARG0(inst);
			uint16_t arg1 = asBC_WORDARG1(inst);
			disas = fmtString("%-8s v%d, v%d", mnem,
				reinterpret_cast<int16_t&>(arg0),
				reinterpret_cast<int16_t&>(arg1));
		}
		break;
		case asBCTYPE_wW_W_ARG: // word dst, word imm
		{
			uint16_t arg0 = asBC_WORDARG0(inst);
			uint16_t arg1 = asBC_WORDARG1(inst);
			disas = fmtString("%-8s v%d, %d", mnem,
				reinterpret_cast<int16_t&>(arg0),
				reinterpret_cast<int16_t&>(arg1));
		}
		break;
		case asBCTYPE_wW_rW_DW_ARG: // word dst, word src, dword imm
		case asBCTYPE_rW_W_DW_ARG:  // word src, word imm, dword imm
		{
			uint16_t arg0 = asBC_WORDARG0(inst);
			uint16_t arg1 = asBC_WORDARG1(inst);
			uint32_t arg2 = asBC_DWORDARG(inst);
			switch (op)
			{
			case asBC_ADDIf:
			case asBC_SUBIf:
			case asBC_MULIf:
				disas = fmtString("%-8s v%d, v%d, %f", mnem,
					reinterpret_cast<int16_t&>(arg0),
					reinterpret_cast<int16_t&>(arg1),
					reinterpret_cast<float&>(arg2));
				break;
			default:
				disas = fmtString("%-8s v%d, v%d, %d", mnem,
					reinterpret_cast<int16_t&>(arg0),
					reinterpret_cast<int16_t&>(arg1),
					reinterpret_cast<int16_t&>(arg2));
				break;
			}
		}
		break;
		case asBCTYPE_DW_ARG: // dword imm
		{
			uint32_t arg0 = asBC_DWORDARG(inst);
			switch (op)
			{
			case asBC_OBJTYPE:
			{
				asIObjectType* type = reinterpret_cast<asIObjectType*>(arg0);
				disas = fmtString("%-8s 0x%x (type:%s)", mnem, arg0, type->GetName());
			}
			break;
			case asBC_FuncPtr:
			{
				asIScriptFunction* func = reinterpret_cast<asIScriptFunction*>(arg0);
				disas = fmtString("%-8s 0x%x (func:%s)", mnem, arg0, func->GetDeclaration());
			}
			break;
			case asBC_PshC4:
			case asBC_Cast:
				disas = fmtString("%-8s 0x%x (i:%d, f:%g)", mnem,
					arg0, reinterpret_cast<int&>(arg0), reinterpret_cast<float&>(arg0));
				break;
			case asBC_TYPEID:
				disas = fmtString("%-8s 0x%x (decl:%s)", mnem,
					arg0, engine->GetTypeDeclaration(reinterpret_cast<int&>(arg0)));
				break;
			case asBC_PGA: // global vars
			case asBC_PshGPtr:
			case asBC_LDG:
			case asBC_PshG4:
			{
				asCGlobalProperty* prop = function->GetPropertyByGlobalVarPtr(reinterpret_cast<void*>(arg0));
				disas = fmtString("%-8s 0x%x (%d:%s)", mnem,
					arg0,
					prop->id,
					prop->name.AddressOf());
			}
			break;
			case asBC_CALL:
			case asBC_CALLSYS:
			case asBC_CALLBND:
			case asBC_CALLINTF:
			case asBC_Thiscall1:
			{
				asIScriptFunction* func;
				if (arg0 & 0x40000000)
				{
					func = engine->importedFunctions[reinterpret_cast<int&>(arg0) & ~0x40000000]->importedFunctionSignature;
				}
				else
				{
					func = engine->GetFunctionById(reinterpret_cast<int&>(arg0));
				}

				disas = fmtString("%-8s %d (%s)", mnem,
					arg0,
					func->GetDeclaration(true, true, true));
			}
			break;
			case asBC_REFCPY:
				disas = fmtString("%-8s 0x%x", mnem, arg0);
				break;
			case asBC_JMP:
			case asBC_JZ:
			case asBC_JLowZ:
			case asBC_JS:
			case asBC_JP:
			case asBC_JNZ:
			case asBC_JLowNZ:
			case asBC_JNS:
			case asBC_JNP:
				disas = fmtString("%-8s %+d (d:%x)", mnem,
					reinterpret_cast<int&>(arg0),
					position + reinterpret_cast<int&>(arg0));
				break;
			default:
				disas = fmtString("%-8s %d", mnem, reinterpret_cast<int&>(arg0));
				break;
			}
		}
		break;
		case asBCTYPE_QW_ARG: // qword imm
		{
			uint64_t arg0 = asBC_QWORDARG(inst);
			// #todo-csasm: Potentially add 64-bit pointer opcode handling
			disas = fmtString("%-8s 0x%llx (i:%lld, f:%g)", mnem,
				arg0, reinterpret_cast<int64_t&>(arg0), reinterpret_cast<double&>(arg0));
		}
		break;
		case asBCTYPE_wW_QW_ARG: // word dst, qword imm
		case asBCTYPE_rW_QW_ARG: // word src, qword imm
		{
			uint16_t arg0 = asBC_WORDARG0(inst);
			uint64_t arg1 = asBC_QWORDARG(inst);
			switch (op)
			{
			case asBC_RefCpyV:
			case asBC_FREE:
			{
				asIObjectType* type = reinterpret_cast<asIObjectType*>(arg1);
				disas = fmtString("%-8s v%d, 0x%x (type:%s)", mnem,
					reinterpret_cast<int16_t&>(arg0),
					static_cast<uint32_t>(arg1),
					type->GetName());
			}
			break;
			default:
				disas = fmtString("%-8s v%d, 0x%llx (i:%lld, f:%g)", mnem,
					reinterpret_cast<int16_t&>(arg0),
					arg1,
					reinterpret_cast<int64_t&>(arg1),
					reinterpret_cast<double&>(arg1));
				break;
			}
		}
		break;
		case asBCTYPE_DW_DW_ARG: // dword imm, dword imm
		{
			// #todo-csasm: Investigate DW_DW layout further
#define asBC_DWORDARG1(x)  (*(((asDWORD*)x)+2))
			uint32_t arg0 = asBC_DWORDARG(inst);
			uint32_t arg1 = asBC_DWORDARG1(inst);
			switch (op)
			{
			case asBC_ALLOC:
			{
				asIObjectType* type = reinterpret_cast<asIObjectType*>(arg0);
				asIScriptFunction* func = engine->GetFunctionById(asBC_WORDARG0(inst));
				disas = fmtString("%-8s 0x%x, %d (type:%s, %s)", mnem,
					arg0,
					reinterpret_cast<int&>(arg1),
					type->GetName(),
					func ? func->GetDeclaration() : "{no func}");
			}
			break;
			case asBC_SetG4:
			{
				asCGlobalProperty* prop = function->GetPropertyByGlobalVarPtr(reinterpret_cast<void*>(arg0));
				disas = fmtString("%-8s 0x%x, %d (%d:%s)", mnem,
					arg0,
					reinterpret_cast<int&>(arg1),
					prop->id,
					prop->name.AddressOf());
			}
			break;
			default:
				disas = fmtString("%-8s %u, %d", mnem,
					arg0,
					reinterpret_cast<int&>(arg1));
				break;
			}
		}
		break;
		case asBCTYPE_rW_DW_DW_ARG: // word src, dword imm, dword imm
		{
			uint16_t arg0 = asBC_WORDARG0(inst);
			uint32_t arg1 = asBC_DWORDARG(inst);
			uint32_t arg2 = asBC_DWORDARG1(inst);

			disas = fmtString("%-8s v%d, %u, %u", mnem,
				reinterpret_cast<int16_t&>(arg0), arg1, arg2);
		}
		break;
		case asBCTYPE_QW_DW_ARG: // qword imm, dword imm
		{
			// #todo-csasm: Consider adding support for 64-bit asBC_ALLOC
			uint64_t arg0 = asBC_QWORDARG(inst);
			uint32_t arg1 = asBC_DWORDARG(inst);

			disas = fmtString("%-8s %llu, %d", mnem,
				reinterpret_cast<int64_t&>(arg0), reinterpret_cast<int&>(arg1));
		}
		break;
		case asBCTYPE_INFO: // word imm
		{
			uint16_t arg0 = asBC_WORDARG0(inst);
			switch (op)
			{
			case asBC_LABEL:
				disas = fmtString("%d:", arg0);
				break;
			case asBC_LINE:
				disas = fmtString("%-8s", mnem);
				break;
			case asBC_Block:
				// #todo-csasm: Add block indenting
				disas = fmtString("%c", arg0 ? '{' : '}');
				break;
			}
		}
		break;
		case asBCTYPE_rW_DW_ARG: // word src, dword imm
		case asBCTYPE_wW_DW_ARG: // word dst, dword imm
		case asBCTYPE_W_DW_ARG: // word imm, dword imm
		{
			uint16_t arg0 = asBC_WORDARG0(inst);
			uint32_t arg1 = asBC_DWORDARG(inst);
			switch (op)
			{
			case asBC_SetV1:
				disas = fmtString("%-8s v%d, 0x%x", mnem,
					reinterpret_cast<int16_t&>(arg0),
					static_cast<uint8_t>(arg1));
				break;
			case asBC_SetV2:
				disas = fmtString("%-8s v%d, 0x%x", mnem,
					reinterpret_cast<int16_t&>(arg0),
					static_cast<uint16_t>(arg1));
				break;
			case asBC_SetV4:
				disas = fmtString("%-8s v%d, 0x%x (i:%d, f:%g)", mnem,
					reinterpret_cast<int16_t&>(arg0),
					arg1,
					reinterpret_cast<int&>(arg1),
					reinterpret_cast<float&>(arg1));
				break;
			case asBC_CMPIf:
				disas = fmtString("%-8s v%d, %f", mnem,
					reinterpret_cast<int16_t&>(arg0),
					reinterpret_cast<float&>(arg1));
				break;
			case asBC_LdGRdR4: // global vars
			case asBC_CpyGtoV4:
			case asBC_CpyVtoV4:
			{
				asCGlobalProperty* prop = function->GetPropertyByGlobalVarPtr(reinterpret_cast<void*>(arg1));
				disas = fmtString("%-8s v%d, 0x%x (%d:%s)", mnem,
					reinterpret_cast<int16_t&>(arg0),
					reinterpret_cast<int&>(arg1),
					prop->id,
					prop->name.AddressOf());
			}
			break;
			default:
				disas = fmtString("%-8s v%d, %d", mnem,
					reinterpret_cast<int16_t&>(arg0),
					reinterpret_cast<int&>(arg1));
				break;
			}
		}
		break;
		case asBCTYPE_wW_rW_rW_ARG: // dst word, src word, src word
		{
			// #todo-csasm: Investigate wW_rW_rW layout further
#define asBC_WORDARG2(x)  (*(((asWORD*)x)+3))
			uint16_t arg0 = asBC_WORDARG0(inst);
			uint16_t arg1 = asBC_WORDARG1(inst);
			uint16_t arg2 = asBC_WORDARG2(inst);
			disas = fmtString("%-8s v%d, v%d, v%d", mnem,
				reinterpret_cast<int16_t&>(arg0),
				reinterpret_cast<int16_t&>(arg1),
				reinterpret_cast<int16_t&>(arg2));
		}
		break;
		default:
			__debugbreak();
			break;
		}
		dump.append(fmtString("%04x: %s\n", position, disas.c_str()));

		position += asBCTypeSize[asBCInfo[op].type];
	}
	return dump;
}

static std::string to_yaml_str(const std::string& input)
{
	// Check if the input string contains ' #', ': ' or a line break because then, it has to be wrapped into quotes
	std::string::size_type commentStart = input.find(" #");
	std::string::size_type colonStart = input.find(": ");
	std::string::size_type lineBreak = input.find("\n");

	if (commentStart != std::string::npos || colonStart != std::string::npos || lineBreak != std::string::npos)
	{
		std::string escaped(input);
		boost::replace_all(escaped, "\\", "\\\\");
		boost::replace_all(escaped, "\n", "\\n");
		boost::replace_all(escaped, "\"", "\\\"");
		return "\"" + escaped + "\"";
	}
	else
	{
		return std::string(input);
	}
}

std::string serializeModuleYaml(asIScriptModule* module, const std::vector<std::string>& dependencies)
{
	// Dump all information in the module
	
	// This function originally used this library https://github.com/jimmiebergmann/mini-yaml.
	// However, I couldn't figure out how it works, so I figured it would be easier
	// to construct the yaml file myself.
	std::stringstream output;

	// Dependencies
	output << "dependencies:\n";
	
	for (const auto& dependency : dependencies)
	{
		output << "  - " << to_yaml_str(dependency) << std::endl;
	}
	
	// Enums
	output << "enums:\n";
	
	for (unsigned int i = 0; i < module->GetEnumCount(); ++i)
	{
		int typeId;
		const char* nameSpace;
		const char* enumName = module->GetEnumByIndex(i, &typeId, &nameSpace);

		std::string prefix;
		if (!std::string(nameSpace).empty())
		{
			prefix = nameSpace;
			prefix += "::";
		}
		
		asIObjectType* type = module->GetEngine()->GetObjectTypeById(typeId);
		std::string name = type
			? fmtString("%s%s : %s", prefix.c_str(), enumName, type->GetName())
			: fmtString("%s%s", prefix.c_str(), enumName);
		
		output << "  " << to_yaml_str(name) << ":\n";
	
		// Values
		for (int j = 0; j < module->GetEnumValueCount(typeId); ++j)
		{
			int value;
			const char* valueName = module->GetEnumValueByIndex(typeId, j, &value);

			output << "    " << to_yaml_str(valueName) << ": " << to_yaml_str(std::to_string(value)) << std::endl;
		}
	}


	// Typedefs
	output << "typedefs:\n";
	
	for (unsigned int i = 0; i < module->GetTypedefCount(); ++i)
	{
		int typeId;
		const char* typedefName = module->GetTypedefByIndex(i, &typeId);

		output << "  " << typeId << ": " << to_yaml_str(typedefName) << std::endl;
	}

	// Object types
	output << "object_types:\n";

	std::unordered_set<std::string> objectTypes;
	
	for (unsigned int i = 0; i < module->GetObjectTypeCount(); ++i)
	{
		asIObjectType* type = module->GetObjectTypeByIndex(i);
		
		std::string name = type->GetName();
		while (objectTypes.count(name) > 0)
		{
			name += "__$";
		}

		objectTypes.insert(name);
		
		output << "  " << to_yaml_str(name) << ":\n";
		
		output << "    size: " << type->GetSize() << std::endl;
		output << "    flags: " << to_yaml_str(fmtString("%08x", type->GetFlags())) << std::endl;
		output << "    properties:\n";
		
		// #todo-csasm: Advanced object type dumping
		// properties
		for (unsigned int j = 0; j < type->GetPropertyCount(); ++j)
		{
			output << "      - " << to_yaml_str(type->GetPropertyDeclaration(j)) << std::endl;
		}
	}

	// Global variables
	output << "global_variables:\n";
	
	for (unsigned int i = 0; i < module->GetGlobalVarCount(); ++i)
	{
		// #todo-csasm: Dump global variables
		output << "  - " << to_yaml_str(module->GetGlobalVarDeclaration(i, true)) << std::endl;
	}

	// Imported functions
	output << "imported_functions:\n";
	
	for (unsigned int i = 0; i < module->GetImportedFunctionCount(); ++i)
	{
		// #todo-csasm: Dump imported functions
		// output["importedFunctions"].push_back({
		// 	{"declaration", module->GetImportedFunctionDeclaration(i)},
		// 	{"origin", module->GetImportedFunctionSourceModule(i)},
		// });

		output << "  - function: " << to_yaml_str(module->GetImportedFunctionDeclaration(i)) << std::endl;
		output << "    origin: " << to_yaml_str(module->GetImportedFunctionSourceModule(i)) << std::endl;
	}

	// Functions
	output << "functions:\n";
	
	for (unsigned int i = 0; i < module->GetFunctionCount(); ++i)
	{
		asIScriptFunction* func = module->GetFunctionByIndex(i);

		output << "  " << to_yaml_str(func->GetDeclaration(true, true, true)) << ":\n";
		
		std::vector<std::string> lines;
		boost::split(lines, serializeBytecode(func), boost::is_any_of("\n"));
		
		for (int i = 0; i < lines.size(); ++i)
		{
			std::string str = lines[i];
			boost::trim_right(str);

			if (str.empty())
				continue;

			const int index = str.find(": ") + 2;
			output << "    - " << to_yaml_str(str.substr(index, str.length() - index)) << std::endl;
		}
	}

	return output.str();
}
