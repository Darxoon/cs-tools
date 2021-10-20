#include "asf.h"

#include "platform.h"
#include "args.h"
#include "text_serialization.h"

#include <angelscript.h>

// for bytecode translation features
#include <../source/as_scriptengine.h>

#include <scriptstdstring/scriptstdstring.h>
#include <scriptarray/scriptarray.h>
#include <scriptany/scriptany.h>
#include <weakref/weakref.h>

#include "json.hpp"

#include <fstream>
#include <iostream>
#include <string>

#include "yaml_serialization.h"

template<typename... A>
static std::string fmtString(const std::string &format, A... args)
{
	// Thread safe
	static thread_local char sFormatBuf[2048];
	snprintf(sFormatBuf, sizeof(sFormatBuf), format.c_str(), args...);
	return std::string(sFormatBuf);
}

void AngelScriptMessageCallback(const asSMessageInfo *msg, void *param)
{
	std::string type;
	if (msg->type == asMSGTYPE_INFORMATION)
	{
		type = "INFO";
	}
	else if (msg->type == asMSGTYPE_WARNING)
	{
		type = "WARN";
	}
	else
	{
		type = "ERR ";
	}
	std::cout << msg->section
	          << " (" << msg->row << ", " << msg->col << ") : "
	          << type << " : " << msg->message << std::endl;
}

void RegisterScriptCafeTypedefs(asIScriptEngine *engine)
{
	engine->RegisterTypedef("f64",      "double");
	engine->RegisterTypedef("f32",      "float");
	engine->RegisterTypedef("s64",      "int64");
	engine->RegisterTypedef("u64",      "uint64");
	engine->RegisterTypedef("s32",      "int");
	engine->RegisterTypedef("u32",      "uint");
	engine->RegisterTypedef("s16",      "int16");
	engine->RegisterTypedef("u16",      "uint16");
	engine->RegisterTypedef("s8",       "int8");
	engine->RegisterTypedef("u8",       "uint8");
	
	engine->RegisterTypedef("uint32_t", "uint");
	engine->RegisterTypedef("int32_t",  "int");
	
	engine->RegisterTypedef("as_f64",   "double");
	engine->RegisterTypedef("as_f32",   "float");
	engine->RegisterTypedef("as_s64",   "int64");
	engine->RegisterTypedef("as_u64",   "uint64");
	engine->RegisterTypedef("as_s32",   "int");
	engine->RegisterTypedef("as_u32",   "uint");
	engine->RegisterTypedef("as_s16",   "int16");
	engine->RegisterTypedef("as_u16",   "uint16");
	engine->RegisterTypedef("as_s8",    "int8");
	engine->RegisterTypedef("as_u8",    "uint8");
	
	engine->RegisterTypedef("AsHandle", "uint64");
	engine->RegisterTypedef("OSTime",   "int64");
}

void RegisterScriptMathTypeDeclarations(asIScriptEngine *engine)
{
	engine->SetDefaultNamespace("math");
	RegisterScriptCafeTypedefs(engine);
	engine->RegisterObjectType("Vec2",   8, asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	engine->RegisterObjectType("Vec3",  16, asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	engine->RegisterObjectType("Vec4",  16, asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	engine->RegisterObjectType("MTX22", 16, asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	engine->RegisterObjectType("MTX23", 24, asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	engine->RegisterObjectType("MTX33", 36, asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	engine->RegisterObjectType("MTX34", 48, asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	engine->RegisterObjectType("MTX43", 48, asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	engine->RegisterObjectType("MTX44", 64, asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	engine->RegisterObjectType("QUAT",  16, asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	engine->SetDefaultNamespace("");
}

//void RegisterMathVEC2(asIScriptEngine *engine)
//{
//	engine->RegisterObjectProperty("Vec2", "as_f32 x", 0);
//	engine->RegisterObjectProperty("Vec2", "as_f32 y", 4);
//
//	engine->RegisterObjectBehaviour("Vec2", asBEHAVE_CONSTRUCT, "void f()",               asFUNCTION(0), asCALL_CDECL_OBJLAST);
//	engine->RegisterObjectBehaviour("Vec2", asBEHAVE_CONSTRUCT, "void f(const Vec2 &in)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
//	engine->RegisterObjectBehaviour("Vec2", asBEHAVE_CONSTRUCT, "void f(as_f32, as_f32)",       asFUNCTION(0), asCALL_CDECL_OBJLAST);
//	engine->RegisterObjectBehaviour("Vec2", asBEHAVE_DESTRUCT,  "void f()",               asFUNCTION(0), asCALL_CDECL_OBJLAST);
//
//	engine->RegisterObjectMethod("Vec2", "const Vec2 &Zero()",                                     asFUNCTION(0), asCALL_CDECL_OBJFIRST);
//	engine->RegisterObjectMethod("Vec2", "Vec2 &opAssign(const Vec2 &in)",                         asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 &opAddAssign(const Vec2 &in)",                      asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 &opSubAssign(const Vec2 &in)",                      asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 &opMulAssign(as_f32)",                                 asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 &opDivAssign(as_f32)",                                 asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 &opNeg() const",                                    asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 opAdd(const Vec2 &in) const",                       asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 opSub(const Vec2 &in) const",                       asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 opMul(as_f32) const",                                  asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 opDiv(as_f32) const",                                  asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 &Lerp(const Vec2 &in, const Vec2 &in, as_f32) const",  asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "as_f32 Dot(const Vec2 &in) const",                          asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "as_f32 LengthSquare() const",                               asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "as_f32 Length() const",                                     asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 &Normalize()",                                      asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 &SetNormalize(const Vec2 &in)",                     asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 &SafeNormalize(const Vec2 &in)",                    asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 &SetSafeNormalize(const Vec2 &in,const Vec2 &in)",  asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "as_f32 DistanceSquare(const Vec2 &in)",                     asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 &SetMaximize(const Vec2 &in, const Vec2 &in)",      asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "Vec2 &SetMinimize(const Vec2 &in, const Vec2 &in)",      asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "void Set(as_f32, as_f32)",                                     asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "bool opEquals(const Vec2 &in) const",                    asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec2", "bool IsZero() const",                                    asFUNCTION(0), asCALL_THISCALL);
//}

//void RegisterMathVEC3(asIScriptEngine *engine)
//{
//	engine->RegisterObjectProperty("Vec3", "as_f32 x", 0);
//	engine->RegisterObjectProperty("Vec3", "as_f32 y", 4);
//	engine->RegisterObjectProperty("Vec3", "as_f32 z", 8);
//
//	engine->RegisterObjectBehaviour("Vec3", asBEHAVE_CONSTRUCT, "void f()",               asFUNCTION(0), asCALL_CDECL_OBJLAST);
//	engine->RegisterObjectBehaviour("Vec3", asBEHAVE_CONSTRUCT, "void f(const Vec3 &in)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
//	engine->RegisterObjectBehaviour("Vec3", asBEHAVE_CONSTRUCT, "void f(as_f32, as_f32, as_f32)",  asFUNCTION(0), asCALL_CDECL_OBJLAST);
//	engine->RegisterObjectBehaviour("Vec3", asBEHAVE_DESTRUCT,  "void f()",               asFUNCTION(0), asCALL_CDECL_OBJLAST);
//
//	engine->RegisterObjectMethod("Vec3", "const Vec3 &Zero()",                                        asFUNCTION(0), asCALL_CDECL_OBJFIRST);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &opAssign(const Vec3 &in)",                            asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &opAddAssign(const Vec3 &in)",                         asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &opSubAssign(const Vec3 &in)",                         asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &opMulAssign(const Vec3 &in)",                         asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &opMulAssign(as_f32)",                                    asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &opDivAssign(const Vec3 &in)",                         asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &opDivAssign(as_f32)",                                    asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &opNeg() const",                                       asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 opAdd(const Vec3 &in) const",                          asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 opSub(const Vec3 &in) const",                          asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 opMul(as_f32) const",                                     asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 opDiv(as_f32) const",                                     asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &Lerp(const Vec3 &in, const Vec3 &in, as_f32) const",     asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "as_f32 Dot(const Vec3 &in) const",                             asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "as_f32 LengthSquare() const",                                  asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "as_f32 Length() const",                                        asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &Normalize()",                                         asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &SetNormalize(const Vec3 &in)",                        asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &SafeNormalize(const Vec3 &in)",                       asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &SetSafeNormalize(const Vec3 &in,const Vec3 &in)",     asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "as_f32 DistanceSquare(const Vec3 &in)",                        asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "as_f32 Distance(const Vec3 &in)",                              asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &SetMaximize(const Vec3 &in, const Vec3 &in)",         asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &SetMinimize(const Vec3 &in, const Vec3 &in)",         asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &Cross(const Vec3 &in)",                               asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &SetCross(const Vec3 &in, const Vec3 &in)",            asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &SetTransform(const MTX33 &in, const Vec3 &in)",       asFUNCTION(0), asCALL_CDECL_OBJFIRST);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &Transform(const MTX33 &in)",                          asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &SetTransform(const MTX34 &in, const Vec3 &in)",       asFUNCTION(0), asCALL_CDECL_OBJFIRST);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &Transform(const MTX34 &in)",                          asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &SetTransform(const MTX44 &in, const Vec3 &in)",       asFUNCTION(0), asCALL_CDECL_OBJFIRST);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &Transform(const MTX44 &in)",                          asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &SetTransformNormal(const MTX34 &in, const Vec3 &in)", asFUNCTION(0), asCALL_CDECL_OBJFIRST);
//	engine->RegisterObjectMethod("Vec3", "Vec3 &TransformNormal(const MTX34 &in)",                    asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "void Set(as_f32, as_f32, as_f32)",                                   asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "void Set(const Vec3 &in)",                                  asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "bool opEquals(const Vec3 &in) const",                       asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec3", "bool IsZero() const",                                       asFUNCTION(0), asCALL_THISCALL);
//}

//void RegisterMathVEC4(asIScriptEngine *engine)
//{
//	engine->RegisterObjectProperty("Vec4", "as_f32 x", 0);
//	engine->RegisterObjectProperty("Vec4", "as_f32 y", 4);
//	engine->RegisterObjectProperty("Vec4", "as_f32 z", 8);
//	engine->RegisterObjectProperty("Vec4", "as_f32 w", 12);
//
//	engine->RegisterObjectBehaviour("Vec4", asBEHAVE_CONSTRUCT, "void f()",                   asFUNCTION(0), asCALL_CDECL_OBJLAST);
//	engine->RegisterObjectBehaviour("Vec4", asBEHAVE_CONSTRUCT, "void f(const Vec4 &in)",     asFUNCTION(0), asCALL_CDECL_OBJLAST);
//	engine->RegisterObjectBehaviour("Vec4", asBEHAVE_CONSTRUCT, "void f(as_f32, as_f32, as_f32, as_f32)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
//	engine->RegisterObjectBehaviour("Vec4", asBEHAVE_DESTRUCT,  "void f()",                   asFUNCTION(0), asCALL_CDECL_OBJLAST);
//
//	engine->RegisterObjectMethod("Vec4", "const Vec4 &Zero()",                                        asFUNCTION(0), asCALL_CDECL_OBJFIRST);
//	engine->RegisterObjectMethod("Vec4", "const Vec4 &ZeroWOne()",                                    asFUNCTION(0), asCALL_CDECL_OBJFIRST);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &opAssign(const Vec4 &in)",                            asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &opAddAssign(const Vec4 &in)",                         asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &opSubAssign(const Vec4 &in)",                         asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &opMulAssign(as_f32)",                                    asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &opDivAssign(as_f32)",                                    asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &opNeg() const",                                       asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 opAdd(const Vec4 &in) const",                          asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 opSub(const Vec4 &in) const",                          asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 opMul(as_f32) const",                                     asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 opDiv(as_f32) const",                                     asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &Lerp(const Vec4 &in, const Vec4 &in, as_f32) const",     asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "as_f32 Dot(const Vec4 &in) const",                             asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "as_f32 LengthSquare() const",                                  asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "as_f32 Length() const",                                        asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &Normalize()",                                         asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &SetNormalize(const Vec4 &in)",                        asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &SafeNormalize(const Vec4 &in)",                       asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &SetSafeNormalize(const Vec4 &in, const Vec4 &in)",    asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "as_f32 DistanceSquare(const Vec4 &in)",                        asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &SetMaximize(const Vec4 &in, const Vec4 &in)",         asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &SetMinimize(const Vec4 &in, const Vec4 &in)",         asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &SetTransform(const MTX34 &in, const Vec4 &in)",       asFUNCTION(0), asCALL_CDECL_OBJFIRST);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &Transform(const MTX34 &in)",                          asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &SetTransform(const MTX44 &in, const Vec4 &in)",       asFUNCTION(0), asCALL_CDECL_OBJFIRST);
//	engine->RegisterObjectMethod("Vec4", "Vec4 &Transform(const MTX44 &in)",                          asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "void Set(as_f32, as_f32, as_f32, as_f32)",                              asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "bool opEquals(const Vec4 &in) const",                       asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "bool IsZero() const",                                       asFUNCTION(0), asCALL_THISCALL);
//	engine->RegisterObjectMethod("Vec4", "bool IsZeroWOne() const",                                   asFUNCTION(0), asCALL_THISCALL);
//}

void RegisterMathMTX22(asIScriptEngine *engine)
{
	engine->RegisterObjectProperty("MTX22", "as_f32 _00", 0);
	engine->RegisterObjectProperty("MTX22", "as_f32 _01", 4);
	engine->RegisterObjectProperty("MTX22", "as_f32 _10", 8);
	engine->RegisterObjectProperty("MTX22", "as_f32 _11", 12);

	engine->RegisterObjectBehaviour("MTX22", asBEHAVE_CONSTRUCT, "void f()",                   asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX22", asBEHAVE_CONSTRUCT, "void f(const MTX22 &in)",    asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX22", asBEHAVE_CONSTRUCT, "void f(as_f32, as_f32, as_f32, as_f32)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX22", asBEHAVE_DESTRUCT,  "void f()",                   asFUNCTION(0), asCALL_CDECL_OBJLAST);

	engine->RegisterObjectMethod("MTX22", "const MTX22& Identity()",              asFUNCTION(0), asCALL_CDECL_OBJFIRST);
	engine->RegisterObjectMethod("MTX22", "MTX22 &opAssign(const MTX22 &in)",     asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX22", "const Vec2 &GetRow(int) const",        asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX22", "Vec2 GetColumn(int) const",            asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX22", "MTX22 &SetIdentity()",                 asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX22", "bool opEquals(const MTX22 &in) const", asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX22", "bool IsIdentity() const",              asFUNCTION(0), asCALL_THISCALL);
}

void RegisterMathMTX23(asIScriptEngine *engine)
{
	engine->RegisterObjectProperty("MTX23", "as_f32 _00", 0);
	engine->RegisterObjectProperty("MTX23", "as_f32 _01", 4);
	engine->RegisterObjectProperty("MTX23", "as_f32 _02", 8);
	engine->RegisterObjectProperty("MTX23", "as_f32 _10", 12);
	engine->RegisterObjectProperty("MTX23", "as_f32 _11", 16);
	engine->RegisterObjectProperty("MTX23", "as_f32 _12", 20);

	engine->RegisterObjectBehaviour("MTX23", asBEHAVE_CONSTRUCT, "void f()",                             asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX23", asBEHAVE_CONSTRUCT, "void f(as_f32, as_f32, as_f32, as_f32, as_f32, as_f32)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX23", asBEHAVE_CONSTRUCT, "void f(const MTX23 &in)",              asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX23", asBEHAVE_DESTRUCT,  "void f()",                             asFUNCTION(0), asCALL_CDECL_OBJLAST);

	engine->RegisterObjectMethod("MTX23", "const MTX23& Identity()",                              asFUNCTION(0), asCALL_CDECL_OBJFIRST);
	engine->RegisterObjectMethod("MTX23", "MTX23 &opAssign(const MTX23 &in)",                     asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX23", "MTX23 &opAddAssign(const MTX23 &in)",                  asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX23", "MTX23 &opSubAssign(const MTX23 &in)",                  asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX23", "MTX23 &opMulAssign(as_f32)",                              asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX23", "MTX23 &opDivAssign(as_f32)",                              asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX23", "MTX23 &opNeg() const",                                 asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX23", "MTX23 opAdd(const MTX23 &in) const",                   asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX23", "MTX23 opSub(const MTX23 &in) const",                   asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX23", "MTX23 opMul(as_f32) const",                               asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX23", "MTX23 opDiv(as_f32) const",                               asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX23", "const Vec3 &GetRow(int) const",                        asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX23", "Vec2 GetColumn(int) const",                            asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX23", "MTX23 &SetIdentity()",                                 asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX23", "MTX23 &SetScale(const MTX23 &in, const Vec2 &in)",     asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX23", "MTX23 &SetTranslate(const MTX23 &in, const Vec2 &in)", asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX23", "MTX23 &SetRotate(as_f32)",                                asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX23", "MTX23 &SetRotate(const Vec2 &in, as_f32)",                asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX23", "bool opEquals(const MTX23 &in) const",                 asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX23", "bool IsIdentity() const",                              asFUNCTION(0), asCALL_THISCALL);
}

void RegisterMathMTX33(asIScriptEngine *engine)
{
	engine->RegisterObjectProperty("MTX33", "as_f32 _00", 0);
	engine->RegisterObjectProperty("MTX33", "as_f32 _01", 4);
	engine->RegisterObjectProperty("MTX33", "as_f32 _02", 8);
	engine->RegisterObjectProperty("MTX33", "as_f32 _10", 12);
	engine->RegisterObjectProperty("MTX33", "as_f32 _11", 16);
	engine->RegisterObjectProperty("MTX33", "as_f32 _12", 20);
	engine->RegisterObjectProperty("MTX33", "as_f32 _20", 24);
	engine->RegisterObjectProperty("MTX33", "as_f32 _21", 28);
	engine->RegisterObjectProperty("MTX33", "as_f32 _22", 32);

	engine->RegisterObjectBehaviour("MTX33", asBEHAVE_CONSTRUCT, "void f()",                                            asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX33", asBEHAVE_CONSTRUCT, "void f(as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX33", asBEHAVE_CONSTRUCT, "void f(const MTX34 &in)",                             asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX33", asBEHAVE_CONSTRUCT, "void f(const MTX33 &in)",                             asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX33", asBEHAVE_DESTRUCT,  "void f()",                                            asFUNCTION(0), asCALL_CDECL_OBJLAST);

	engine->RegisterObjectMethod("MTX33", "const MTX33& Identity()",              asFUNCTION(0), asCALL_CDECL_OBJFIRST);
	engine->RegisterObjectMethod("MTX33", "const Vec3 &GetRow(int) const",        asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX33", "Vec3 GetColumn(int) const",            asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX33", "MTX33 &SetIdentity()",                 asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX33", "bool opEquals(const MTX33 &in) const", asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX33", "bool IsIdentity() const",              asFUNCTION(0), asCALL_THISCALL);
}

void RegisterMathMTX34(asIScriptEngine *engine)
{
	engine->RegisterObjectProperty("MTX34", "as_f32 _00", 0);
	engine->RegisterObjectProperty("MTX34", "as_f32 _01", 4);
	engine->RegisterObjectProperty("MTX34", "as_f32 _02", 8);
	engine->RegisterObjectProperty("MTX34", "as_f32 _03", 12);
	engine->RegisterObjectProperty("MTX34", "as_f32 _10", 16);
	engine->RegisterObjectProperty("MTX34", "as_f32 _11", 20);
	engine->RegisterObjectProperty("MTX34", "as_f32 _12", 24);
	engine->RegisterObjectProperty("MTX34", "as_f32 _13", 28);
	engine->RegisterObjectProperty("MTX34", "as_f32 _20", 32);
	engine->RegisterObjectProperty("MTX34", "as_f32 _21", 36);
	engine->RegisterObjectProperty("MTX34", "as_f32 _22", 40);
	engine->RegisterObjectProperty("MTX34", "as_f32 _23", 44);

	engine->RegisterObjectBehaviour("MTX34", asBEHAVE_CONSTRUCT, "void f()",                                                           asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX34", asBEHAVE_CONSTRUCT, "void f(const MTX33 &in)",                                            asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX34", asBEHAVE_CONSTRUCT, "void f(as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX34", asBEHAVE_CONSTRUCT, "void f(const MTX34 &in)",                                            asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX34", asBEHAVE_DESTRUCT,  "void f()",                                                           asFUNCTION(0), asCALL_CDECL_OBJLAST);

	engine->RegisterObjectMethod("MTX34", "const MTX34& Identity()",                                                         asFUNCTION(0), asCALL_CDECL_OBJFIRST);
	engine->RegisterObjectMethod("MTX34", "MTX34 &opAssign(const MTX34 &in)",                                                asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &opAddAssign(const MTX34 &in)",                                             asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &opSubAssign(const MTX34 &in)",                                             asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &opMulAssign(as_f32)",                                                         asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &opDivAssign(as_f32)",                                                         asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &opNeg() const",                                                            asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 opAdd(const MTX34 &in) const",                                              asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX34", "MTX34 opSub(const MTX34 &in) const",                                              asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX34", "MTX34 opMul(as_f32) const",                                                          asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX34", "MTX34 opMul_r(as_f32) const",                                                        asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX34", "MTX34 opDiv(as_f32) const",                                                          asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX34", "const Vec4 &GetRow(int) const",                                                   asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "Vec3 GetColumn(int) const",                                                       asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX34", "MTX34 &SetIdentity()",                                                            asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &SetScale(const Vec3 &in)",                                                 asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &SetTranslate(const Vec3 &in)",                                             asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &SetRotateXyz(const Vec3 &in)",                                             asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &SetRotate(const Vec3 &in, as_f32)",                                           asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &SetRotate(const QUAT &in)",                                                asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &SetLookAt(const Vec3 &in, const Vec3 &in, const Vec3 &in)",                asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &SetLookAt(const Vec3 &in, as_f32, const Vec3 &in)",                           asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &SetCameraRotate(const Vec3 &in, const Vec3 &in)",                          asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &SetTextureProjectionFrustum(as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32)", asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &SetTextureProjectionPerspective(as_f32, as_f32, as_f32, as_f32, as_f32, as_f32)",            asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "MTX34 &SetTextureProjectionOrtho(as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32)",        asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "bool opEquals(const MTX34 &in) const",                                            asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX34", "bool IsIdentity() const",                                                         asFUNCTION(0), asCALL_THISCALL);
}

void RegisterMathMTX43(asIScriptEngine *engine)
{
	engine->RegisterObjectProperty("MTX43", "as_f32 _00", 0);
	engine->RegisterObjectProperty("MTX43", "as_f32 _01", 4);
	engine->RegisterObjectProperty("MTX43", "as_f32 _02", 8);
	engine->RegisterObjectProperty("MTX43", "as_f32 _10", 12);
	engine->RegisterObjectProperty("MTX43", "as_f32 _11", 16);
	engine->RegisterObjectProperty("MTX43", "as_f32 _12", 20);
	engine->RegisterObjectProperty("MTX43", "as_f32 _20", 24);
	engine->RegisterObjectProperty("MTX43", "as_f32 _21", 28);
	engine->RegisterObjectProperty("MTX43", "as_f32 _22", 32);
	engine->RegisterObjectProperty("MTX43", "as_f32 _30", 36);
	engine->RegisterObjectProperty("MTX43", "as_f32 _31", 40);
	engine->RegisterObjectProperty("MTX43", "as_f32 _32", 44);

	engine->RegisterObjectBehaviour("MTX43", asBEHAVE_CONSTRUCT, "void f()",                                                           asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX43", asBEHAVE_CONSTRUCT, "void f(as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX43", asBEHAVE_CONSTRUCT, "void f(const MTX43 &in)",                                            asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX43", asBEHAVE_DESTRUCT,  "void f()",                                                           asFUNCTION(0), asCALL_CDECL_OBJLAST);

	engine->RegisterObjectMethod("MTX43", "const MTX43& Identity()",             asFUNCTION(0), asCALL_CDECL_OBJFIRST);
	engine->RegisterObjectMethod("MTX43", "MTX43 &opAssign(const MTX43 &in)",    asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX43", "MTX43 &opAddAssign(const MTX43 &in)", asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX43", "MTX43 &opSubAssign(const MTX43 &in)", asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX43", "MTX43 &opMulAssign(const MTX43 &in)", asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX43", "MTX43 &opMulAssign(as_f32)",             asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX43", "MTX43 &opDivAssign(as_f32)",             asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX43", "MTX43 &opNeg() const",                asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX43", "MTX43 opAdd(const MTX43 &in) const",  asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX43", "MTX43 opSub(const MTX43 &in) const",  asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX43", "MTX43 opMul(as_f32) const",              asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX43", "MTX43 opDiv(as_f32) const",              asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX43", "const Vec3 &GetRow(int) const",       asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX43", "Vec4 GetColumn(int) const",           asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX43", "MTX43 &SetIdentity()",                asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX43", "bool opEquals(const MTX43 &in) const",asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX43", "bool IsIdentity() const",             asFUNCTION(0), asCALL_THISCALL);
}

void RegisterMathMTX44(asIScriptEngine *engine)
{
	engine->RegisterEnum("PivotDirection");
	engine->RegisterEnumValue("PivotDirection", "PIVOT_NONE",             0);
	engine->RegisterEnumValue("PivotDirection", "PIVOT_UPSIDE_TO_TOP",    1);
	engine->RegisterEnumValue("PivotDirection", "PIVOT_UPSIDE_TO_RIGHT",  2);
	engine->RegisterEnumValue("PivotDirection", "PIVOT_UPSIDE_TO_BOTTOM", 3);
	engine->RegisterEnumValue("PivotDirection", "PIVOT_UPSIDE_TO_LEFT",   4);
	engine->RegisterEnumValue("PivotDirection", "PIVOT_NUM",              5);

	engine->RegisterObjectProperty("MTX44", "as_f32 _00", 0);
	engine->RegisterObjectProperty("MTX44", "as_f32 _01", 4);
	engine->RegisterObjectProperty("MTX44", "as_f32 _02", 8);
	engine->RegisterObjectProperty("MTX44", "as_f32 _03", 12);
	engine->RegisterObjectProperty("MTX44", "as_f32 _10", 16);
	engine->RegisterObjectProperty("MTX44", "as_f32 _11", 20);
	engine->RegisterObjectProperty("MTX44", "as_f32 _12", 24);
	engine->RegisterObjectProperty("MTX44", "as_f32 _13", 28);
	engine->RegisterObjectProperty("MTX44", "as_f32 _20", 32);
	engine->RegisterObjectProperty("MTX44", "as_f32 _21", 36);
	engine->RegisterObjectProperty("MTX44", "as_f32 _22", 40);
	engine->RegisterObjectProperty("MTX44", "as_f32 _23", 44);
	engine->RegisterObjectProperty("MTX44", "as_f32 _30", 48);
	engine->RegisterObjectProperty("MTX44", "as_f32 _31", 52);
	engine->RegisterObjectProperty("MTX44", "as_f32 _32", 56);
	engine->RegisterObjectProperty("MTX44", "as_f32 _33", 60);
	
	engine->RegisterObjectBehaviour("MTX44", asBEHAVE_CONSTRUCT, "void f()",                                                                               asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX44", asBEHAVE_CONSTRUCT, "void f(const MTX34 &in)",                                                                asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX44", asBEHAVE_CONSTRUCT, "void f(as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, as_f32)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX44", asBEHAVE_CONSTRUCT, "void f(const MTX44 &in)",                                                                asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("MTX44", asBEHAVE_DESTRUCT,  "void f()",                                                                               asFUNCTION(0), asCALL_CDECL_OBJLAST);
	
	engine->RegisterObjectMethod("MTX44", "const MTX44& Identity()",                                         asFUNCTION(0), asCALL_CDECL_OBJFIRST);
	engine->RegisterObjectMethod("MTX44", "MTX44 &opAssign(const MTX44 &in)",                                asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &opAddAssign(const MTX44 &in)",                             asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &opSubAssign(const MTX44 &in)",                             asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &opMulAssign(const MTX44 &in)",                             asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &opMulAssign(as_f32)",                                         asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &opDivAssign(as_f32)",                                         asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &opNeg() const",                                            asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 opAdd(const MTX44 &in) const",                              asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX44", "MTX44 opSub(const MTX44 &in) const",                              asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX44", "MTX44 opMul(as_f32) const",                                          asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX44", "MTX44 opDiv(as_f32) const",                                          asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX44", "const Vec4 &GetRow(int) const",                                   asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "Vec4 GetColumn(int) const",                                       asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("MTX44", "MTX44 &Transpose()",                                              asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &SetIdentity()",                                            asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &SetScale(const Vec3 &in)",                                 asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &SetTranslate(const Vec3 &in)",                             asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &SetRotateXyz(const Vec3 &in)",                             asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &SetRotate(const Vec3 &in, as_f32)",                           asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &SetFrustum(as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, PivotDirection)", asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &SetOrtho(as_f32, as_f32, as_f32, as_f32, as_f32, as_f32, PivotDirection)",   asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "MTX44 &SetPerspective(as_f32, as_f32, as_f32, as_f32, PivotDirection)",       asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "bool opEquals(const MTX44 &in) const",                            asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("MTX44", "bool IsIdentity() const",                                         asFUNCTION(0), asCALL_THISCALL);

}

void RegisterMathQUAT(asIScriptEngine *engine)
{
	engine->RegisterObjectProperty("QUAT", "as_f32 x", 0);
	engine->RegisterObjectProperty("QUAT", "as_f32 y", 4);
	engine->RegisterObjectProperty("QUAT", "as_f32 z", 8);
	engine->RegisterObjectProperty("QUAT", "as_f32 w", 12);

	engine->RegisterObjectBehaviour("QUAT", asBEHAVE_CONSTRUCT, "void f()",                   asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("QUAT", asBEHAVE_CONSTRUCT, "void f(const QUAT &in)",     asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("QUAT", asBEHAVE_CONSTRUCT, "void f(as_f32, as_f32, as_f32, as_f32)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("QUAT", asBEHAVE_DESTRUCT,  "void f()",                   asFUNCTION(0), asCALL_CDECL_OBJLAST);

	engine->RegisterObjectMethod("QUAT", "QUAT &opAssign(const QUAT &in)",                            asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("QUAT", "QUAT &opAddAssign(const QUAT &in)",                         asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("QUAT", "QUAT &opSubAssign(const QUAT &in)",                         asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("QUAT", "QUAT &opMulAssign(as_f32)",                                    asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("QUAT", "QUAT &opDivAssign(as_f32)",                                    asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("QUAT", "QUAT &opNeg() const",                                       asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("QUAT", "QUAT opAdd(const QUAT &in) const",                          asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("QUAT", "QUAT opSub(const QUAT &in) const",                          asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("QUAT", "QUAT opMul(as_f32) const",                                     asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("QUAT", "QUAT opDiv(as_f32) const",                                     asFUNCTION(0), asCALL_THISCALL);
	engine->RegisterObjectMethod("QUAT", "bool opEquals(const QUAT &in) const",                       asFUNCTION(0), asCALL_THISCALL);

	engine->RegisterGlobalFunction("QUAT& QUATAdd(QUAT &out, const QUAT &in, const QUAT &in )",                                       asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATSub(QUAT &out, const QUAT &in, const QUAT &in )",                                       asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATDivide(QUAT &out, const QUAT &in, const QUAT &in )",                                    asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATMult(QUAT &out, const QUAT &in, const QUAT &in )",                                      asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("as_f32 QUATDot(const QUAT &in, const QUAT &in )",                                                    asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATInverse(QUAT &out, const QUAT &in )",                                                   asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATScale(QUAT&out, const QUAT&in , as_f32 )",                                                 asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATNormalize(QUAT&out, const QUAT&in )",                                                   asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATExp(QUAT&out, const QUAT&in )",                                                         asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATLogN(QUAT&out, const QUAT&in )",                                                        asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATLerp(QUAT&out, const QUAT&in , const QUAT&in , as_f32 )",                                  asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATSlerp(QUAT&out, const QUAT&in , const QUAT&in , as_f32 )",                                 asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& MTX34ToQUAT(QUAT&out, const MTX34&in )",                                                    asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATSquad(QUAT&out, const QUAT&in , const QUAT&in , const QUAT&in , const QUAT&in , as_f32 )", asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATMakeClosest(QUAT&out, const QUAT&in , const QUAT&in  )",                                asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATRotAxisRad(QUAT&out, const Vec3&in , as_f32  )",                                           asFUNCTION(0), asCALL_CDECL);
	engine->RegisterGlobalFunction("QUAT& QUATMakeVectorRotation(QUAT&out, const Vec3&in , const Vec3&in  )",                         asFUNCTION(0), asCALL_CDECL);
}

void RegisterScriptMathTypeDefinitions(asIScriptEngine *engine)
{
	engine->SetDefaultNamespace("math");
	//RegisterMathVEC2(engine);
	//RegisterMathVEC3(engine);
	//RegisterMathVEC4(engine);
	RegisterMathMTX22(engine);
	RegisterMathMTX23(engine);
	RegisterMathMTX33(engine);
	RegisterMathMTX34(engine);
	RegisterMathMTX43(engine);
	RegisterMathMTX44(engine);
	RegisterMathQUAT(engine);
	engine->SetDefaultNamespace("");
}

void RegisterScriptMathTypes(asIScriptEngine *engine)
{
	RegisterScriptMathTypeDeclarations(engine);
	RegisterScriptMathTypeDefinitions(engine);
}

void RegisterScriptPreregNamespaces(asIScriptEngine *engine, const nlohmann::json &config)
{
	for (const auto &registration : config["namespaces"])
	{
		engine->SetDefaultNamespace(registration["namespace"].get<std::string>().c_str());
		RegisterScriptCafeTypedefs(engine);
		engine->SetDefaultNamespace("");
	}
}

void RegisterScriptPreregEnumsAndValues(asIScriptEngine *engine, const nlohmann::json &config)
{
	// Unused in PMCS
}

void RegisterScriptPreregObjectTypes(asIScriptEngine *engine, const nlohmann::json &config)
{
	for (const auto &registration : config["object_types"])
	{
		engine->SetDefaultNamespace(registration["namespace"].get<std::string>().c_str());
		engine->RegisterObjectType(registration["object_name"].get<std::string>().c_str(),
								   registration["size"].get<int>(),
								   registration["flags"].get<asDWORD>());
		engine->SetDefaultNamespace("");
	}
}

void RegisterScriptPreregObjectProperties(asIScriptEngine *engine, const nlohmann::json &config)
{
	for (const auto &registration : config["object_properties"])
	{
		engine->SetDefaultNamespace(registration["namespace"].get<std::string>().c_str());
		engine->RegisterObjectProperty(registration["object_name"].get<std::string>().c_str(),
									   registration["declaration"].get<std::string>().c_str(),
									   registration["offset"].get<int>());
		engine->SetDefaultNamespace("");
	}
}

void RegisterScriptPreregObjectBehaviours(asIScriptEngine *engine, const nlohmann::json &config)
{
	for (const auto &registration : config["object_behaviours"])
	{
		engine->SetDefaultNamespace(registration["namespace"].get<std::string>().c_str());
		engine->RegisterObjectBehaviour(registration["object_name"].get<std::string>().c_str(),
										registration["behaviour"].get<asEBehaviours>(),
										registration["declaration"].get<std::string>().c_str(),
										asFUNCTION(0),
										registration["calling_convention"].get<asECallConvTypes>());
		engine->SetDefaultNamespace("");
	}
}

void RegisterScriptPreregObjectMethods(asIScriptEngine *engine, const nlohmann::json &config)
{
	for (const auto &registration : config["object_methods"])
	{
		engine->SetDefaultNamespace(registration["namespace"].get<std::string>().c_str());
		//std::cout << "registration " << registration << std::endl;
		engine->RegisterObjectMethod(registration["object_name"].get<std::string>().c_str(),
									 registration["declaration"].get<std::string>().c_str(),
									 asFUNCTION(0),
									 registration["calling_convention"].get<int>());
		engine->SetDefaultNamespace("");
	}
}

void RegisterScriptPreregGlobalFunctions(asIScriptEngine *engine, const nlohmann::json &config)
{
	for (const auto &registration : config["global_functions"])
	{
		engine->SetDefaultNamespace(registration["namespace"].get<std::string>().c_str());
		engine->RegisterGlobalFunction(registration["declaration"].get<std::string>().c_str(),
									   asFUNCTION(0),
									   asCALL_CDECL);
		engine->SetDefaultNamespace("");
	}
}

void RegisterScriptPreregGlobalProperties(asIScriptEngine *engine, const nlohmann::json &config)
{

	for (const auto &registration : config["global_properties"])
	{
		engine->SetDefaultNamespace(registration["namespace"].get<std::string>().c_str());
		engine->RegisterGlobalProperty(registration["declaration"].get<std::string>().c_str(),
									   reinterpret_cast<void *>(static_cast<size_t>(engine->GetGlobalPropertyCount() + 1)));
		engine->SetDefaultNamespace("");
	}
}

void RegisterScriptPreregFuncDefs(asIScriptEngine* engine, const nlohmann::json& config)
{
	for (const auto& registration : config["funcdefs"])
	{
		engine->SetDefaultNamespace(registration["namespace"].get<std::string>().c_str());
		//std::cout << "registration " << registration << std::endl;
		//engine->RegisterObjectMethod(registration["object_name"].get<std::string>().c_str(),
		//	registration["declaration"].get<std::string>().c_str(),
		//	asFUNCTION(0),
		//	registration["calling_convention"].get<int>());
		engine->RegisterFuncdef(registration["declaration"].get<std::string>().c_str());
		engine->SetDefaultNamespace("");
	}
}

void ConfigureEngine(asIScriptEngine *engine, const nlohmann::json &config)
{
	// Replicate the PMCS scripting environment

	// Init PMCS global properties
	// PMCS has this at true, but we're building, so we have to diverge here
	engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, 0);
	engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, 1);
	engine->SetEngineProperty(asEP_SCRIPT_SCANNER, 1);
	engine->SetEngineProperty(asEP_STRING_ENCODING, 0);
	engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, 1);
	engine->SetEngineProperty(asEP_AUTO_GARBAGE_COLLECT, 1);

	// Register script extensions
	RegisterStdString(engine);
	engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(const int)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(const uint)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(const bool)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(const float)", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	RegisterScriptArray(engine, false);
	RegisterStdStringUtils(engine);
	RegisterScriptAny(engine);
	RegisterScriptWeakRef(engine);
	
	// PMCS-specific extensions
	RegisterScriptCafeTypedefs(engine);
	RegisterScriptMathTypes(engine);

	// The rest of this comes from the data through a linked list built through
	// global constructors; we parse this as a JSON
	RegisterScriptPreregNamespaces(engine, config);
	RegisterScriptPreregEnumsAndValues(engine, config);
	RegisterScriptPreregObjectTypes(engine, config);
	RegisterScriptPreregFuncDefs(engine, config);
	RegisterScriptPreregObjectProperties(engine, config);
	RegisterScriptPreregObjectBehaviours(engine, config);
	RegisterScriptPreregObjectMethods(engine, config);
	RegisterScriptPreregGlobalFunctions(engine, config);
	RegisterScriptPreregGlobalProperties(engine, config);
}

namespace fs = boost::filesystem;

static fs::path get_module_path(fs::path root, std::string module_path)
{
	fs::path p(root);
	
	if (root.string()[root.size() - 1] != '/' && root.string()[root.size() - 1] != '\\')
		p.append("/");

	p.append(module_path);

	if (fs::exists(p))
		return module_path;
	else
		return fs::relative(fs::absolute(module_path), fs::absolute(root));
}

int main(int argc, char **argv)
{
	// ConIO for UTF8 characters
	setupConsoleCodePage();
	
	auto args = parseArgs(argc, argv);
	if (!args.valid)
		return 0;

	std::cout << fmtString("csasm by PistonMiner, built on %s\n\n", __TIMESTAMP__);

	bool verbose = args.verbose;
	
	// Create engine
	asIScriptEngine *engine = asCreateScriptEngine();
	if (!engine)
	{
		return -1;
	}

	try {
		engine->SetMessageCallback(asFUNCTION(AngelScriptMessageCallback), 0, asCALL_CDECL);

		// We must replicate the scripting environment that PMCS registers in order to parse its scripts
		if (!exists(args.configFile))
		{
			throw file_not_found(args.configFile.string(), "Registry");
		}
		
		std::ifstream configStream(args.configFile.c_str());
		nlohmann::json config = nlohmann::json::parse(configStream);
		ConfigureEngine(engine, config);

		AsfModuleTracker tracker(engine, args.rootFolder.string());
		std::vector<std::string> dependencies;
		fs::path modulePath = get_module_path(args.rootFolder, args.modulePath);
		AsfModule* mainModule = tracker.getModule(modulePath.string(), &dependencies, verbose);

		if (args.dumpFile.empty() && args.outputFile.empty() && args.binaryOutputFile.empty())
			std::cout << dumpModule(mainModule->getScriptModule(), dependencies);
		if (!args.outputFile.empty())
		{
			fs::path file(args.outputFile);
			fs::create_directories(fs::absolute(file).parent_path());

			fs::ofstream stream(file);
			stream << serializeModuleYaml(mainModule->getScriptModule(), dependencies);
			stream.close();
		}
		if (!args.dumpFile.empty())
		{
			fs::path file(args.dumpFile);
			fs::create_directories(fs::absolute(file).parent_path());

			fs::ofstream stream(file);
			stream << dumpModule(mainModule->getScriptModule(), dependencies);
			stream.close();
		}

		if (!args.binaryOutputFile.empty())
		{
			std::vector<uint8_t> data = mainModule->save();
			
			fs::path file(args.binaryOutputFile);
			fs::create_directories(fs::absolute(file).parent_path());

			fs::ofstream stream(file, std::ios::binary);
			stream.write(reinterpret_cast<const char*>(data.data()), data.size());
			stream.close();
		}
	}
	catch (file_not_found& e)
	{
		namespace fs = boost::filesystem;
		
		std::cout << e.what() << std::endl;
		if (fs::exists(fs::path(e.m_filepath + ".zst")))
		{
			std::cout << "Perhaps you forgot to decompress the scripts. To do that, use \"Decrypt all\" from https://github.com/Darxoon/TOKElfTool";
		}

		return -1;
	}

	resetConsoleCodePage();
}