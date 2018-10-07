#pragma once

#include <stdint.h>

enum shader_param_type
{
  SHADER_PARAM_TYPE_Int,
  SHADER_PARAM_TYPE_Bool,
  SHADER_PARAM_TYPE_Float,
  SHADER_PARAM_TYPE_Vec3,
  SHADER_PARAM_TYPE_Vec4,
  SHADER_PARAM_TYPE_Map,

  SHADER_PARAM_TYPE_Count,
};

struct shader_param_def
{
  int32_t Type;
  int32_t OffsetIntoMaterial;
};

struct named_shader_param_def
{
  char*   Name;
  int32_t Type;
  int32_t OffsetIntoMaterial;
};

struct shader_def* AddShaderDef(int32_t ShaderType, const char* ShaderName);
void AddParamDef(struct shader_def* ShaderDefPtr, const char* ParamName, shader_param_def ParamDef);
// seturns false on error
int32_t GetShaderType(const struct shader_def* ShaderDef);
bool    GetShaderDef(struct shader_def** OutputDefPtrPtr, const char* ShaderName);
bool    GetShaderDef(struct shader_def** OutputDefPtrPtr, int32_t ShaderType);
bool    GetShaderParamDef(shader_param_def* OutputDefPtr, const shader_def* ShaderDefPtr,
                          const char* ParamName);
bool    ResetShaderDefIterator(struct shader_def* ShaderDer);
bool    GetNextShaderParam(named_shader_param_def* NamedDef, struct shader_def* OutputDefPtr);
