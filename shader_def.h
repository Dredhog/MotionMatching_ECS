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
  char*   UniformName;
  int32_t Type;
  int32_t OffsetIntoMaterial;
};

struct shader_def* AddShaderDef(int32_t ShaderType, uint32_t ShaderID, const char* ShaderName);
void AddParamDef(struct shader_def* ShaderDefPtr, const char* ParamName, const char* UniformName,
                 shader_param_def ParamDef);

// asserts if if shader def is null
int32_t     GetShaderType(const struct shader_def* ShaderDef);
uint32_t    GetShaderID(const struct shader_def* ShaderDef);
const char* GetShaderName(const struct shader_def* ShaderDef);
void        ResetShaderDefIterator(struct shader_def* ShaderDef);

// seturns false on error
bool GetShaderDef(struct shader_def** OutputDefPtrPtr, const char* ShaderName);
bool GetShaderDef(struct shader_def** OutputDefPtrPtr, int32_t ShaderType);
bool GetShaderParamDef(shader_param_def* OutputDefPtr, const shader_def* ShaderDefPtr,
                       const char* ParamName);
bool GetNextShaderParam(named_shader_param_def* NamedDef, struct shader_def* ShaderDef);
